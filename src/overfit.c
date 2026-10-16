#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char endpoint[1024];
    char model[256];
    char api_key[1024];
} Config;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

static void fail(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static void buffer_init(Buffer *buffer)
{
    buffer->capacity = 4096;
    buffer->length = 0;
    buffer->data = calloc(buffer->capacity, 1);
    if (buffer->data == NULL) {
        fail("out of memory");
    }
}

static void buffer_append(Buffer *buffer, const char *data, size_t length)
{
    while (buffer->length + length + 1 > buffer->capacity) {
        buffer->capacity *= 2;
        buffer->data = realloc(buffer->data, buffer->capacity);
        if (buffer->data == NULL) {
            fail("out of memory");
        }
    }
    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
}

static void buffer_free(Buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
}

static void append_json_string(Buffer *buffer, const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;
    buffer_append(buffer, "\"", 1);
    while (*cursor != '\0') {
        char escaped[7];
        switch (*cursor) {
        case '\\': buffer_append(buffer, "\\\\", 2); break;
        case '"': buffer_append(buffer, "\\\"", 2); break;
        case '\n': buffer_append(buffer, "\\n", 2); break;
        case '\r': buffer_append(buffer, "\\r", 2); break;
        case '\t': buffer_append(buffer, "\\t", 2); break;
        default:
            if (*cursor < 32) {
                snprintf(escaped, sizeof(escaped), "\\u%04x", *cursor);
                buffer_append(buffer, escaped, strlen(escaped));
            } else {
                buffer_append(buffer, (const char *)cursor, 1);
            }
        }
        cursor++;
    }
    buffer_append(buffer, "\"", 1);
}

static int run_process(char *const argv[], Buffer *output)
{
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        fail("pipe: %s", strerror(errno));
    }
    pid_t pid = fork();
    if (pid < 0) {
        fail("fork: %s", strerror(errno));
    }
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            _exit(126);
        }
        if (dup2(pipefd[1], STDERR_FILENO) < 0) {
            _exit(126);
        }
        close(pipefd[1]);
        execvp(argv[0], argv);
        _exit(127);
    }
    close(pipefd[1]);
    buffer_init(output);
    char chunk[4096];
    ssize_t count;
    while ((count = read(pipefd[0], chunk, sizeof(chunk))) > 0) {
        buffer_append(output, chunk, (size_t)count);
    }
    close(pipefd[0]);
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        fail("waitpid: %s", strerror(errno));
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 128 + WTERMSIG(status);
}

static int run_passthrough(char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0) {
        fail("fork: %s", strerror(errno));
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        fail("waitpid: %s", strerror(errno));
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 128 + WTERMSIG(status);
}

static void home_path(char *path, size_t size)
{
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        fail("HOME is not set");
    }
    if (snprintf(path, size, "%s/.config/overfit/config", home) >= (int)size) {
        fail("configuration path is too long");
    }
}

static int read_line(FILE *stream, char *destination, size_t size)
{
    if (fgets(destination, (int)size, stream) == NULL) {
        return 0;
    }
    destination[strcspn(destination, "\r\n")] = '\0';
    return 1;
}

static void load_config(Config *config)
{
    char path[PATH_MAX];
    home_path(path, sizeof(path));
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fail("configuration not found; run `overfit setup`");
    }
    memset(config, 0, sizeof(*config));
    char line[2048];
    while (read_line(file, line, sizeof(line))) {
        char *separator = strchr(line, '=');
        if (separator == NULL) {
            continue;
        }
        *separator = '\0';
        const char *value = separator + 1;
        if (strcmp(line, "endpoint") == 0) {
            snprintf(config->endpoint, sizeof(config->endpoint), "%s", value);
        } else if (strcmp(line, "model") == 0) {
            snprintf(config->model, sizeof(config->model), "%s", value);
        } else if (strcmp(line, "api_key") == 0) {
            snprintf(config->api_key, sizeof(config->api_key), "%s", value);
        }
    }
    fclose(file);
    if (config->endpoint[0] == '\0' || config->model[0] == '\0' || config->api_key[0] == '\0') {
        fail("configuration is incomplete; run `overfit setup`");
    }
}

static void setup_config(int argc, char **argv)
{
    char endpoint[1024] = "https://api.openai.com/v1/chat/completions";
    char model[256] = "gpt-4o-mini";
    char api_key[1024] = "";
    for (int index = 2; index < argc; index++) {
        if (strcmp(argv[index], "--endpoint") == 0 && index + 1 < argc) {
            snprintf(endpoint, sizeof(endpoint), "%s", argv[++index]);
        } else if (strcmp(argv[index], "--model") == 0 && index + 1 < argc) {
            snprintf(model, sizeof(model), "%s", argv[++index]);
        } else if (strcmp(argv[index], "--api-key") == 0 && index + 1 < argc) {
            snprintf(api_key, sizeof(api_key), "%s", argv[++index]);
        } else {
            fail("unknown setup option: %s", argv[index]);
        }
    }
    if (api_key[0] == '\0') {
        fprintf(stdout, "API key: ");
        fflush(stdout);
        if (!read_line(stdin, api_key, sizeof(api_key))) {
            fail("could not read API key");
        }
    }
    char directory[PATH_MAX];
    char config_root[PATH_MAX];
    char path[PATH_MAX];
    home_path(path, sizeof(path));
    const char *home = getenv("HOME");
    if (home == NULL) {
        fail("HOME is not set");
    }
    if (snprintf(config_root, sizeof(config_root), "%s/.config", home) >= (int)sizeof(config_root)) {
        fail("configuration path is too long");
    }
    const char *slash = strrchr(path, '/');
    size_t directory_length = (size_t)(slash - path);
    memcpy(directory, path, directory_length);
    directory[directory_length] = '\0';
    if (mkdir(config_root, 0700) != 0 && errno != EEXIST) {
        fail("mkdir %s: %s", config_root, strerror(errno));
    }
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) {
        fail("mkdir %s: %s", directory, strerror(errno));
    }
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        fail("open %s: %s", path, strerror(errno));
    }
    if (fchmod(fileno(file), 0600) != 0) {
        fclose(file);
        fail("protect %s: %s", path, strerror(errno));
    }
    fprintf(file, "endpoint=%s\nmodel=%s\napi_key=%s\n", endpoint, model, api_key);
    fclose(file);
    printf("Saved %s with mode 0600.\n", path);
}

static void collect_repo(Buffer *snapshot)
{
    char *status[] = {"git", "status", "--short", NULL};
    char *diff[] = {"git", "diff", "--no-ext-diff", NULL};
    char *cached_diff[] = {"git", "diff", "--cached", "--no-ext-diff", NULL};
    char *untracked[] = {"git", "ls-files", "--others", "--exclude-standard", NULL};
    Buffer status_output, diff_output, cached_diff_output, untracked_output;
    if (run_process(status, &status_output) != 0) {
        fail("git status failed: %s", status_output.data);
    }
    if (run_process(diff, &diff_output) != 0) {
        fail("git diff failed: %s", diff_output.data);
    }
    if (run_process(cached_diff, &cached_diff_output) != 0) {
        fail("git diff --cached failed: %s", cached_diff_output.data);
    }
    if (run_process(untracked, &untracked_output) != 0) {
        fail("git ls-files failed: %s", untracked_output.data);
    }
    buffer_init(snapshot);
    buffer_append(snapshot, "STATUS:\n", 8);
    buffer_append(snapshot, status_output.data, status_output.length);
    buffer_append(snapshot, "\nDIFF:\n", 8);
    buffer_append(snapshot, diff_output.data, diff_output.length);
    buffer_append(snapshot, "\nSTAGED DIFF:\n", 14);
    buffer_append(snapshot, cached_diff_output.data, cached_diff_output.length);
    buffer_append(snapshot, "\nUNTRACKED PATHS:\n", 18);
    buffer_append(snapshot, untracked_output.data, untracked_output.length);
    if (snapshot->length > 120000) {
        snapshot->data[120000] = '\0';
        snapshot->length = 120000;
    }
    buffer_free(&status_output);
    buffer_free(&diff_output);
    buffer_free(&cached_diff_output);
    buffer_free(&untracked_output);
}

static char *json_value(const char *json, const char *key)
{
    char needle[256];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *cursor = strstr(json, needle);
    if (cursor == NULL) {
        return NULL;
    }
    cursor = strchr(cursor + strlen(needle), ':');
    if (cursor == NULL) {
        return NULL;
    }
    cursor++;
    while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t') {
        cursor++;
    }
    if (*cursor != '"') {
        return NULL;
    }
    cursor++;
    Buffer value;
    buffer_init(&value);
    int escaped = 0;
    while (*cursor != '\0') {
        if (!escaped && *cursor == '"') {
            return value.data;
        }
        if (!escaped && *cursor == '\\') {
            escaped = 1;
        } else if (escaped) {
            switch (*cursor) {
            case 'n': buffer_append(&value, "\n", 1); break;
            case 'r': buffer_append(&value, "\r", 1); break;
            case 't': buffer_append(&value, "\t", 1); break;
            default: buffer_append(&value, cursor, 1); break;
            }
            escaped = 0;
        } else {
            buffer_append(&value, cursor, 1);
        }
        cursor++;
    }
    buffer_free(&value);
    return NULL;
}

static char *ask_ai(const Config *config, const char *instruction, const char *snapshot)
{
    Buffer request;
    buffer_init(&request);
    buffer_append(&request, "{\"model\":", 9);
    append_json_string(&request, config->model);
    buffer_append(&request, ",\"temperature\":0.1,\"messages\":[{\"role\":\"system\",\"content\":", 75);
    append_json_string(&request, instruction);
    buffer_append(&request, "},{\"role\":\"user\",\"content\":", 35);
    append_json_string(&request, snapshot);
    buffer_append(&request, "}]}\n", 4);
    char auth[1200];
    char endpoint[1024];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
    snprintf(endpoint, sizeof(endpoint), "%s", config->endpoint);
    char *curl[] = {"curl", "--silent", "--show-error", "--fail-with-body", "--max-time", "90", "-H", "Content-Type: application/json", "-H", auth, "--data-binary", request.data, endpoint, NULL};
    Buffer response;
    int result = run_process(curl, &response);
    buffer_free(&request);
    if (result != 0) {
        fail("AI request failed: %s", response.data);
    }
    char *content = json_value(response.data, "content");
    if (content == NULL) {
        fail("AI response did not contain message content");
    }
    buffer_free(&response);
    return content;
}

static int pending_changes(void)
{
    char *status[] = {"git", "status", "--porcelain", NULL};
    Buffer output;
    int result = run_process(status, &output);
    if (result != 0) {
        fail("git status failed: %s", output.data);
    }
    int pending = output.length > 0;
    buffer_free(&output);
    return pending;
}

static void require_git_repository(void)
{
    char *root[] = {"git", "rev-parse", "--show-toplevel", NULL};
    Buffer output;
    if (run_process(root, &output) != 0) {
        fail("not inside a git repository");
    }
    buffer_free(&output);
}

static char *commit_message(const Config *config, const Buffer *snapshot)
{
    const char *instruction = "Return JSON exactly as {\"commit_message\":\"type(scope): imperative summary\"}. Choose a Conventional Commits type and optional scope from the code changes. Use a concise imperative subject, no period, and no markdown. Do not invent work not present in the snapshot.";
    char *response = ask_ai(config, instruction, snapshot->data);
    char *message = json_value(response, "commit_message");
    free(response);
    if (message == NULL) {
        fail("AI returned invalid commit metadata");
    }
    if (strchr(message, '\n') != NULL || strlen(message) > 200 || strchr(message, '\r') != NULL) {
        free(message);
        fail("AI returned an invalid commit message");
    }
    return message;
}

static char *current_branch(void);

static void do_commit(int set_upstream)
{
    require_git_repository();
    if (!pending_changes()) {
        printf("No pending changes.\n");
        return;
    }
    Config config;
    load_config(&config);
    Buffer snapshot;
    collect_repo(&snapshot);
    char *message = commit_message(&config, &snapshot);
    char *add[] = {"git", "add", "--all", NULL};
    if (run_passthrough(add) != 0) {
        free(message);
        buffer_free(&snapshot);
        fail("git add failed");
    }
    char *commit[] = {"git", "commit", "-m", message, NULL};
    int result = run_passthrough(commit);
    free(message);
    buffer_free(&snapshot);
    if (result != 0) {
        fail("git commit failed");
    }
    char *branch = NULL;
    char *push[] = {"git", "push", NULL};
    char *upstream_push[] = {"git", "push", "--set-upstream", "origin", NULL, NULL};
    if (set_upstream) {
        branch = current_branch();
        upstream_push[4] = branch;
    }
    int push_result = run_passthrough(set_upstream ? upstream_push : push);
    free(branch);
    if (push_result != 0) {
        fail("git push failed");
    }
}

static int countdown(void)
{
    if (!isatty(STDIN_FILENO)) {
        return 1;
    }
    struct termios original_terminal;
    if (tcgetattr(STDIN_FILENO, &original_terminal) != 0) {
        fail("could not read terminal settings: %s", strerror(errno));
    }
    struct termios raw_terminal = original_terminal;
    raw_terminal.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw_terminal.c_cc[VMIN] = 0;
    raw_terminal.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_terminal) != 0) {
        fail("could not configure terminal input: %s", strerror(errno));
    }
    printf("Pending changes. Committing in 5 seconds. Press any key to skip.\n");
    for (int seconds = 5; seconds > 0; seconds--) {
        printf("%d... ", seconds);
        fflush(stdout);
        struct pollfd descriptor = {.fd = STDIN_FILENO, .events = POLLIN};
        int ready = poll(&descriptor, 1, 1000);
        if (ready > 0 && (descriptor.revents & POLLIN) != 0) {
            char input;
            if (read(STDIN_FILENO, &input, sizeof(input)) > 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
                printf("\nSkipped automatic commit.\n");
                return 0;
            }
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
    putchar('\n');
    return 1;
}

static char *current_branch(void)
{
    char *branch[] = {"git", "branch", "--show-current", NULL};
    Buffer output;
    if (run_process(branch, &output) != 0 || output.length == 0) {
        fail("could not determine current branch");
    }
    output.data[strcspn(output.data, "\r\n")] = '\0';
    return output.data;
}

static void do_pr(int argc, char **argv)
{
    require_git_repository();
    int skip_commit = 0;
    for (int index = 2; index < argc; index++) {
        if (strcmp(argv[index], "--skip-commit") == 0 || strcmp(argv[index], "-sc") == 0) {
            skip_commit = 1;
        } else {
            fail("unknown pr option: %s", argv[index]);
        }
    }
    if (pending_changes() && !skip_commit && countdown()) {
        do_commit(1);
    }
    Config config;
    load_config(&config);
    Buffer snapshot;
    collect_repo(&snapshot);
    const char *instruction = "Return JSON exactly as {\"title\":\"short imperative PR title\",\"body\":\"markdown PR body with Summary and Testing sections\"}. Base both fields only on the repository snapshot. Use a concise title and describe actual changes and tests without inventing results.";
    char *response = ask_ai(&config, instruction, snapshot.data);
    char *title = json_value(response, "title");
    char *body = json_value(response, "body");
    free(response);
    if (title == NULL || body == NULL || title[0] == '\0' || body[0] == '\0') {
        free(title);
        free(body);
        buffer_free(&snapshot);
        fail("AI returned invalid PR metadata");
    }
    char *branch = current_branch();
    char *push[] = {"git", "push", "--set-upstream", "origin", branch, NULL};
    if (run_passthrough(push) != 0) {
        free(title);
        free(body);
        free(branch);
        buffer_free(&snapshot);
        fail("git push failed");
    }
    char *create[] = {"gh", "pr", "create", "--title", title, "--body", body, "--head", branch, NULL};
    int result = run_passthrough(create);
    free(title);
    free(body);
    free(branch);
    buffer_free(&snapshot);
    if (result != 0) {
        fail("gh pr create failed");
    }
}

static void usage(void)
{
    puts("Usage: overfit <setup|commit|pr> [options]");
    puts("  overfit setup [--endpoint URL] [--model MODEL] [--api-key KEY]");
    puts("  overfit commit");
    puts("  overfit pr [--skip-commit|-sc]");
}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage();
        return argc < 2 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "setup") == 0) {
        setup_config(argc, argv);
    } else if (strcmp(argv[1], "commit") == 0) {
        if (argc != 2) {
            fail("commit does not accept options");
        }
        do_commit(0);
    } else if (strcmp(argv[1], "pr") == 0) {
        do_pr(argc, argv);
    } else {
        usage();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
