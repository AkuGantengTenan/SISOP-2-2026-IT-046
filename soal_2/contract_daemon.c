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

/* =========================================================
   HELPER: tulis ke log dengan newline
   ========================================================= */
void write_log(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");   /* "a" = append, tidak menimpa */
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

/* =========================================================
   HELPER: ambil timestamp sekarang sebagai string
   ========================================================= */
void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

/* =========================================================
   HELPER: pilih status acak
   ========================================================= */
const char *get_random_status() {
    return statuses[rand() % 3];
}

/* =========================================================
   FUNGSI: buat contract.txt (pertama kali / fresh)
   ========================================================= */
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

/* =========================================================
   FUNGSI: restore contract.txt setelah dihapus
   ========================================================= */
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

    /* Update original_contract agar tidak dianggap "diubah" */
    snprintf(original_contract, sizeof(original_contract),
        "\"A promise to keep going, even when unseen.\"\n\nrestored at: %s\n",
        timestamp);
}

/* =========================================================
   FUNGSI: cek kondisi contract.txt
   Kembalikan: 0 = OK, 1 = dihapus, 2 = diubah
   ========================================================= */
int check_contract() {
    /* Cek apakah file ada */
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

/* =========================================================
   SIGNAL HANDLER: tangani SIGTERM (kill / systemctl stop)
   ========================================================= */
volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig;  /* suppress unused warning */
    write_log("We really weren't meant to be together");
    running = 0;
}

/* =========================================================
   FUNGSI: daemonize — ubah proses menjadi daemon
   ========================================================= */
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

/* =========================================================
   MAIN
   ========================================================= */
int main() {
    /* Inisialisasi random seed */
    srand((unsigned int)time(NULL));

    /* Jadikan proses ini daemon */
    daemonize();

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
