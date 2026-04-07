#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define PID_FILE   "/tmp/angel.pid"
#define LOG_FILE   "ethereal.log"
#define LOVE_FILE  "LoveLetter.txt"

const char *sentences[] = {
    "aku akan fokus pada diriku sendiri",
    "aku mencintaimu dari sekarang hingga selamanya",
    "aku akan menjauh darimu, hingga takdir mempertemukan kita di versi kita yang terbaik.",
    "kalau aku dilahirkan kembali, aku tetap akan terus menyayangimu"
};
const int sentence_count = 4;

void write_log(const char *process_name, const char *status) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(f, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
        t->tm_hour, t->tm_min, t->tm_sec,
        process_name, status);

    fclose(f);
}

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const unsigned char *data, size_t input_length) {
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded = malloc(output_length + 1);
    if (!encoded) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded[j++] = b64_table[(triple >> 18) & 0x3F];
        encoded[j++] = b64_table[(triple >> 12) & 0x3F];
        encoded[j++] = b64_table[(triple >>  6) & 0x3F];
        encoded[j++] = b64_table[(triple >>  0) & 0x3F];
    }

    // Padding
    size_t mod = input_length % 3;
    if (mod == 1) { encoded[output_length-1] = '='; encoded[output_length-2] = '='; }
    else if (mod == 2) { encoded[output_length-1] = '='; }
    encoded[output_length] = '\0';
    return encoded;
}

char *base64_decode(const char *data, size_t input_length, size_t *output_length) {
    // Buat lookup table
    int dtable[256];
    memset(dtable, -1, sizeof(dtable));
    for (int i = 0; i < 64; i++) dtable[(unsigned char)b64_table[i]] = i;
    dtable['='] = 0;

    // Hitung panjang output
    *output_length = input_length / 4 * 3;
    if (input_length >= 1 && data[input_length-1] == '=') (*output_length)--;
    if (input_length >= 2 && data[input_length-2] == '=') (*output_length)--;

    char *decoded = malloc(*output_length + 1);
    if (!decoded) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 : dtable[(unsigned char)data[i]]; i++;
        uint32_t sextet_b = data[i] == '=' ? 0 : dtable[(unsigned char)data[i]]; i++;
        uint32_t sextet_c = data[i] == '=' ? 0 : dtable[(unsigned char)data[i]]; i++;
        uint32_t sextet_d = data[i] == '=' ? 0 : dtable[(unsigned char)data[i]]; i++;
        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        if (j < *output_length) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < *output_length) decoded[j++] = (triple >>  8) & 0xFF;
        if (j < *output_length) decoded[j++] = (triple >>  0) & 0xFF;
    }
    decoded[*output_length] = '\0';
    return decoded;
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exit

    if (setsid() < 0) exit(EXIT_FAILURE);

    // Fork kedua agar tidak bisa acquire terminal
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir(".");

    // Tutup stdin/stdout/stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Simpan PID ke file
    FILE *pf = fopen(PID_FILE, "w");
    if (pf) {
        fprintf(pf, "%d\n", getpid());
        fclose(pf);
    }
}

void *secret(void *arg) {
    (void)arg;
    srand(time(NULL));

    while (1) {
        write_log("secret", "RUNNING");

        int idx = rand() % sentence_count;
        FILE *f = fopen(LOVE_FILE, "w");
        if (f) {
            fprintf(f, "%s\n", sentences[idx]);
            fclose(f);
            write_log("secret", "SUCCESS");
        } else {
            write_log("secret", "ERROR");
        }

        sleep(10);
    }
    return NULL;
}

void *surprise(void *arg) {
    (void)arg;

    while (1) {
        sleep(11); // Tunggu setelah secret menulis (10 detik + buffer)

        write_log("surprise", "RUNNING");

        FILE *f = fopen(LOVE_FILE, "r");
        if (!f) {
            write_log("surprise", "ERROR");
            continue;
        }

        // Baca isi file
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        rewind(f);
        char *content = malloc(fsize + 1);
        fread(content, 1, fsize, f);
        content[fsize] = '\0';
        fclose(f);

        // Enkripsi dengan Base64
        char *encoded = base64_encode((unsigned char *)content, strlen(content));
        free(content);

        // Tulis kembali ke file
        f = fopen(LOVE_FILE, "w");
        if (f) {
            fprintf(f, "%s\n", encoded);
            fclose(f);
            write_log("surprise", "SUCCESS");
        } else {
            write_log("surprise", "ERROR");
        }
        free(encoded);
    }
    return NULL;
}

void do_decrypt() {
    write_log("decrypt", "RUNNING");

    FILE *f = fopen(LOVE_FILE, "r");
    if (!f) {
        fprintf(stderr, "Error: File %s tidak ditemukan.\n", LOVE_FILE);
        write_log("decrypt", "ERROR");
        return;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);
    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    content[fsize] = '\0';
    fclose(f);

    // Hapus newline di akhir
    size_t len = strlen(content);
    while (len > 0 && (content[len-1] == '\n' || content[len-1] == '\r')) {
        content[--len] = '\0';
    }

    size_t out_len;
    char *decoded = base64_decode(content, len, &out_len);
    free(content);

    if (!decoded) {
        write_log("decrypt", "ERROR");
        return;
    }

    f = fopen(LOVE_FILE, "w");
    if (f) {
        fprintf(f, "%s", decoded);
        fclose(f);
        printf("Berhasil didekripsi: %s\n", decoded);
        write_log("decrypt", "SUCCESS");
    } else {
        write_log("decrypt", "ERROR");
    }
    free(decoded);
}

void do_kill() {
    write_log("kill", "RUNNING");

    FILE *pf = fopen(PID_FILE, "r");
    if (!pf) {
        fprintf(stderr, "Error: Daemon belum berjalan.\n");
        write_log("kill", "ERROR");
        return;
    }

    pid_t pid;
    fscanf(pf, "%d", &pid);
    fclose(pf);

    if (kill(pid, SIGTERM) == 0) {
        remove(PID_FILE);
        printf("Daemon (PID %d) berhasil dihentikan.\n", pid);
        write_log("kill", "SUCCESS");
    } else {
        fprintf(stderr, "Gagal menghentikan daemon.\n");
        write_log("kill", "ERROR");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Penggunaan:\n");
        printf("  ./angel -daemon  : jalankan sebagai daemon (nama proses: maya)\n");
        printf("  ./angel -decrypt : decrypt LoveLetter.txt\n");
        printf("  ./angel -kill    : kill proses\n");
        return 0;
    }

    if (strcmp(argv[1], "-daemon") == 0) {
        daemonize();

        // Ubah nama proses menjadi "maya"
        strncpy(argv[0], "maya", strlen(argv[0]));

        // Jalankan kedua thread
        pthread_t t_secret, t_surprise;
        pthread_create(&t_secret,  NULL, secret,   NULL);
        pthread_create(&t_surprise, NULL, surprise, NULL);

        pthread_join(t_secret,  NULL);
        pthread_join(t_surprise, NULL);

    } else if (strcmp(argv[1], "-decrypt") == 0) {
        do_decrypt();

    } else if (strcmp(argv[1], "-kill") == 0) {
        do_kill();

    } else {
        printf("Argumen tidak dikenal: %s\n", argv[1]);
        printf("Gunakan ./angel tanpa argumen untuk melihat penggunaan.\n");
    }

    return 0;
}


