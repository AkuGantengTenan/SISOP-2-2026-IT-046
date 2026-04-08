# SISOP-2-2026

<details>
<summary>Soal 1</summary>

**Penjelasan**

Pertama buat file program c bernama kasir_muthu.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

void cek_status(int status) {
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("[ERROR] Aiyaa! Proses gagal, file atau folder tidak ditemukan.\n");
        exit(1);
    }
}
```

Inisialisasi header file yang dibutuhkan, kemudian Fungsi cek_status() bertugas sebagai penjaga keamanan setiap langkah program. Setiap kali sebuah child process selesai bekerja, parent process memanggil fungsi ini untuk memastikan pekerjaan child berhasil. Fungsi menerima parameter status yang merupakan nilai kembalian dari waitpid(). Di dalamnya, WIFEXITED(status) mengecek apakah child selesai secara normal, dan WEXITSTATUS(status) mengambil exit code-nya — jika exit code bukan 0, berarti terjadi kegagalan. 

```c
int main() {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("mkdir", "mkdir", "brankas_kedai", NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);
```

Di dalam main(), program mendeklarasikan variabel pid bertipe pid_t untuk menyimpan hasil fork(), dan variabel status bertipe int untuk menampung status child setelah selesai. Kemudian program memanggil fork() untuk pertama kalinya, yang menduplikasi proses menjadi dua — parent (Upin) dan child (Ipin). Jika nilai pid kurang dari 0, berarti fork() gagal dan program berhenti. Jika pid sama dengan 0, berarti ini adalah child process, lalu execlp() dipanggil untuk menjalankan perintah mkdir brankas_kedai guna membuat folder brankas. 

```c
    pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("cp", "cp", "buku_hutang.csv", "brankas_kedai/", NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);
```

parent melakukan fork() kedua kalinya untuk memanggil child process kedua. Child process ini bertugas menyalin file buku_hutang.csv ke dalam folder brankas_kedai/ menggunakan perintah cp melalui execlp(). Polanya sama persis dengan child pertama — jika fork() gagal program berhenti, jika ini child maka jalankan execlp(), dan parent menunggu dengan waitpid() lalu mengecek status.

```c
 pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("bash", "bash", "-c",
            "grep \"Belum Lunas\" brankas_kedai/buku_hutang.csv "
            "> brankas_kedai/daftar_penunggak.txt",
            NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);
```

Child process ketiga bertugas mencari semua baris yang mengandung teks "Belum Lunas" dari file buku_hutang.csv yang ada di dalam brankas_kedai, lalu menyimpan hasilnya ke file daftar_penunggak.txt. Karena perintah ini melibatkan operator redirect > yang merupakan fitur shell dan tidak bisa langsung dijalankan oleh execlp(), maka digunakan trik bash -c. Dengan cara ini, bash yang akan menginterpretasikan redirect > tersebut, sehingga output grep berhasil ditulis ke file tujuan

```c
    pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("zip", "zip", "-r", "rahasia_muthu.zip", "brankas_kedai", NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);

    printf("[INFO] Fuhh, selamat! Buku hutang dan daftar penagihan berhasil diamankan.\n");

    return 0;
}
```

Child process keempat dan terakhir bertugas mengunci brankas dengan cara mengompres seluruh folder brankas_kedai menjadi sebuah file arsip bernama rahasia_muthu.zip menggunakan perintah zip -r. Flag -r berarti recursive, artinya semua file dan subfolder di dalam brankas_kedai ikut dikompres. 

**Output**



**Kendala**

Tidak ada kendala

</details>


<details>
<summary>Soal 2</summary>

**Penjelasan** 

Buat file C dengan nama daemon contract 

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

/* ─── Path file ─────────────────────────────────────────── */
#define LOG_FILE      "work.log"
#define CONTRACT_FILE "contract.txt"

// SESUDAH (path absolut - ganti sesuai output pwd kamu)
#define LOG_FILE      "/home/wongirengberbulu/SISOP-2-2026-IT-046/soal_2/work.log"
#define CONTRACT_FILE "/home/wongirengberbulu/SISOP-2-2026-IT-046/soal_2/contract.txt"

/* ─── Isi awal contract.txt ──────────────────────────────── */
#define CONTRACT_CONTENT \
    "\"A promise to keep going, even when unseen.\"\n\ncreated at: "

/* ─── Status acak ────────────────────────────────────────── */
const char *statuses[] = {"awake", "drifting", "numbness"};

/* ─── Simpan isi asli contract untuk deteksi perubahan ───── */
char original_contract[512];
```

Bagian awal kode mendefinisikan library standar yang dibutuhkan untuk operasi input-output, manipulasi file, penanganan sinyal, dan manajemen proses. Di sini juga ditentukan nama file yang akan dikelola, yaitu work.log untuk mencatat aktivitas dan contract.txt sebagai file utama yang dipantau. Terdapat juga sebuah larik (array) statuses yang berisi kata-kata puitis untuk memberikan variasi pada isi log, serta sebuah variabel global original_contract yang berfungsi sebagai memori jangka pendek untuk menyimpan isi asli file guna mendeteksi perubahan.

```c

void write_log(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");   /* "a" = append, tidak menimpa */
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

const char *get_random_status() {
    return statuses[rand() % 3];
}
```

Terdapat tiga fungsi pembantu utama: write_log, get_timestamp, dan get_random_status. Fungsi write_log bertugas membuka file log dalam mode append agar pesan baru tidak menghapus pesan lama, memastikan setiap aktivitas terekam secara kronologis. get_timestamp mengambil waktu sistem saat ini dan memformatnya menjadi teks yang mudah dibaca, sementara get_random_status memberikan elemen variasi acak pada log agar aktivitas daemon tidak terlihat monoton saat dipantau.

```c
void create_contract() {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    FILE *f = fopen(CONTRACT_FILE, "w");
    if (f) {
        fprintf(f,
            "\"A promise to keep going, even when unseen.\"\n\ncreated at: %s\n",
            timestamp);
        fclose(f);
    }

    /* Simpan isi asli untuk perbandingan nanti */
    snprintf(original_contract, sizeof(original_contract),
        "\"A promise to keep going, even when unseen.\"\n\ncreated at: %s\n",
        timestamp);
}

void restore_contract() {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    FILE *f = fopen(CONTRACT_FILE, "w");
    if (f) {
        fprintf(f,
            "\"A promise to keep going, even when unseen.\"\n\nrestored at: %s\n",
            timestamp);
        fclose(f);
    }

     if (access(CONTRACT_FILE, F_OK) != 0) {
        return 1;   /* FILE DIHAPUS */
    }

    /* Baca isi file saat ini */
    FILE *f = fopen(CONTRACT_FILE, "r");
    if (!f) return 1;

    char current[512] = {0};
    fread(current, 1, sizeof(current) - 1, f);
    fclose(f);

    /* Bandingkan dengan isi asli */
    if (strcmp(current, original_contract) != 0) {
        return 2;   /* ISI BERUBAH */
    }

    return 0;   /* NORMAL */
}

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig;  /* suppress unused warning */
    write_log("We really weren't meant to be together");
    running = 0;
}
```

Bagian ini terdiri dari fungsi create_contract, restore_contract, dan check_contract. Inti dari logika program ada di sini: create_contract membuat file awal dengan stempel waktu, sementara check_contract secara berkala memverifikasi apakah file tersebut masih ada atau isinya telah berubah dengan cara membandingkannya dengan original_contract. Jika terjadi pelanggaran (file dihapus atau diedit), fungsi restore_contract akan segera dipanggil untuk menulis ulang file tersebut ke kondisi semula, sehingga file seolah-olah tidak bisa dihancurkan.

```c
void daemonize() {
    pid_t pid;

    /* Fork #1 */
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);  /* fork gagal */
    if (pid > 0) exit(EXIT_SUCCESS);  /* parent exit */

    /* Child jadi session leader */
    if (setsid() < 0) exit(EXIT_FAILURE);

    /* Fork #2 — pastikan bukan session leader */
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    /* Set permission file */
    umask(0);

    /* Pindah ke root agar tidak lock direktori */
    chdir("/");

    /* Tutup file descriptor standar */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Arahkan fd 0,1,2 ke /dev/null */
    open("/dev/null", O_RDONLY);   /* fd 0 = stdin  */
    open("/dev/null", O_WRONLY);   /* fd 1 = stdout */
    open("/dev/null", O_WRONLY);   /* fd 2 = stderr */
}
```

Fungsi daemonize adalah bagian yang mengubah program biasa menjadi proses latar belakang yang mandiri. Proses ini dilakukan melalui teknik double-fork: fork pertama melepaskan program dari terminal, setsid() membuat sesi baru agar proses tidak mati saat terminal ditutup, dan fork kedua memastikan daemon tidak bisa mengambil alih terminal kembali. Selain itu, fungsi ini menutup saluran input/output standar (stdin, stdout, stderr) dan mengalihkannya ke /dev/null agar tidak ada pesan yang bocor ke layar pengguna.

```c
    /* Daftarkan signal handler untuk SIGTERM */
    signal(SIGTERM, signal_handler);

    /* Buat contract.txt saat pertama jalan */
    create_contract();

    /* Loop utama */
    while (running) {
        /* Tulis "still working…" ke log */
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg),
            "still working... [%s]", get_random_status());
        write_log(log_msg);

        /* Tunggu 5 detik (dengan cek setiap 1 detik agar respons cepat) */
        for (int i = 0; i < 5 && running; i++) {
            sleep(1);

            /* Cek contract setiap detik agar restore < 2 detik */
            int status = check_contract();

            if (status == 1) {
                /* File dihapus → restore */
                restore_contract();

            } else if (status == 2) {
                /* File diubah → log + restore */
                write_log("contract violated.");
                restore_contract();
            }
        }
    }

    return EXIT_SUCCESS;
}
```

Bagian terakhir adalah fungsi main dan signal_handler. Program mendaftarkan sinyal SIGTERM agar ketika daemon dihentikan (misalnya melalui perintah kill), ia sempat menuliskan pesan perpisahan ke log sebelum benar-benar mati. Di dalam while(running), program menjalankan loop abadi yang melakukan pemantauan setiap satu detik. Jeda waktu ini diatur sedemikian rupa agar program sangat responsif dalam memulihkan file (kurang dari 2 detik) namun tetap hemat sumber daya CPU karena adanya fungsi sleep.

**Output**

**Kendala**
</details>

<details>
<summary>Soal 3</summary>

**Penjelasan**

```c

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
```

p

```c
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
```

p

```c
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

```

p

```c
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

```

p

```c
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

```

p

```c
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

```

p

**Output**

**Kendala**
</details>
