#define _DEFAULT_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define LINE_LEN 256
#define NAME_LEN 256
#define PATH_LEN 512

static int b64_value(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    if (c == '=') {
        return -2;
    }
    if (c == '\r' || c == '\n') {
        return -3;
    }
    return -1;
}

static int decode_base64_line(const char *line, FILE *out)
{
    int val[4];
    int pad[4];
    unsigned q = 0;

    for (const char *p = line; *p; p++) {
        int v = b64_value((unsigned char)*p);

        if (v == -3) {
            continue;
        }
        if (v < -2) {
            fprintf(stderr, "invalid base64 character 0x%02x\n",
                    (unsigned char)*p);
            return -1;
        }

        val[q] = v < 0 ? 0 : v;
        pad[q] = v == -2;
        q++;

        if (q == 4) {
            unsigned triple = ((unsigned)val[0] << 18) |
                              ((unsigned)val[1] << 12) |
                              ((unsigned)val[2] << 6) |
                              (unsigned)val[3];

            fputc((triple >> 16) & 0xff, out);
            if (!pad[2]) {
                fputc((triple >> 8) & 0xff, out);
            }
            if (!pad[3]) {
                fputc(triple & 0xff, out);
            }
            q = 0;
        }
    }

    return q == 0 ? 0 : -1;
}

static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

int main(int argc, char **argv)
{
    const char *log_path;
    const char *out_dir = ".";
    FILE *log;
    FILE *out = NULL;
    char line[LINE_LEN];
    unsigned frames = 0;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s screenlog.0 [output_dir]\n", argv[0]);
        return 2;
    }

    log_path = argv[1];
    if (argc == 3) {
        out_dir = argv[2];
        if (mkdir(out_dir, 0777) < 0) {
            struct stat st;
            if (stat(out_dir, &st) < 0 || !S_ISDIR(st.st_mode)) {
                perror("mkdir output_dir");
                return 1;
            }
        }
    }

    log = fopen(log_path, "rb");
    if (!log) {
        perror("fopen log");
        return 1;
    }

    while (fgets(line, sizeof(line), log)) {
        if (!out) {
            unsigned index;
            char name[NAME_LEN];

            if (sscanf(line, "BEGIN_FRAME %u %255s", &index, name) == 2) {
                char path[PATH_LEN];
                int n = snprintf(path, sizeof(path), "%s/%s", out_dir,
                                 base_name(name));

                if (n < 0 || (size_t)n >= sizeof(path)) {
                    fprintf(stderr, "output path too long\n");
                    fclose(log);
                    return 1;
                }

                out = fopen(path, "wb");
                if (!out) {
                    perror("fopen output");
                    fclose(log);
                    return 1;
                }
                printf("decoding frame %u -> %s\n", index, path);
            }
            continue;
        }

        if (strncmp(line, "END_FRAME", 9) == 0) {
            fclose(out);
            out = NULL;
            frames++;
            continue;
        }

        if (decode_base64_line(line, out) < 0) {
            fprintf(stderr, "base64 decode failed\n");
            fclose(out);
            fclose(log);
            return 1;
        }
    }

    if (out) {
        fprintf(stderr, "log ended inside a frame block\n");
        fclose(out);
        fclose(log);
        return 1;
    }

    fclose(log);
    printf("decoded %u frame(s)\n", frames);
    return 0;
}
