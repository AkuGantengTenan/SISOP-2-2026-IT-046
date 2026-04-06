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

int main() {
    pid_t pid;
    int status;

    // CHILD 1: mkdir
    pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("mkdir", "mkdir", "brankas_kedai", NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);

    // CHILD 2: cp
    pid = fork();
    if (pid < 0) { perror("fork gagal"); exit(1); }
    if (pid == 0) {
        execlp("cp", "cp", "buku_hutang.csv", "brankas_kedai/", NULL);
        exit(1);
    }
    waitpid(pid, &status, 0);
    cek_status(status);

    // CHILD 3: grep → daftar_penunggak.txt
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

    // CHILD 4: zip
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
