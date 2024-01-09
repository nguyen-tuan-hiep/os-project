#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SHM_SIZE 1024  // Kích thước của vùng nhớ chia sẻ
#define SEM_NAME "/my_semaphore"
#define RECEIVE_DIR "receive_files"

int main(int argc, char *argv[]) {
    int shmid;
    key_t key;
    char *shmaddr;
    sem_t *sem;

    // Tạo hoặc truy cập vùng nhớ chia sẻ
    key = ftok(".", 'S');  // Sử dụng chung key như sender
    if ((shmid = shmget(key, SHM_SIZE, 0666)) == -1) {
        perror("shmget");
        exit(1);
    }

    // Kết nối vùng nhớ chia sẻ vào không gian bộ nhớ của tiến trình
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // Tạo semaphore hoặc sử dụng semaphore đã tạo
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    int receive_file = 0;

    char filename[SHM_SIZE];

    // Tạo thư mục receive_files
    mkdir(RECEIVE_DIR, 0777);

    while (1) {
        printf("Menu:\n");
        printf("1. Receive a message\n");
        printf("2. Receive a file\n");
        printf("3. Quit\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                // Đợi cho đến khi có thông điệp sẵn sàng
                sem_wait(sem);
                printf("Received message: %s", shmaddr);
                break;

            case 2:
                // Đợi cho đến khi có file sẵn sàng
                sem_wait(sem);

                // Tạo tên file mới trong thư mục receive_files
                sprintf(filename, "%s/%d.txt", RECEIVE_DIR, receive_file);

                // Mở tệp tin để ghi nội dung từ shared memory
                FILE *file = fopen(filename, "w");
                if (file == NULL) {
                    perror("fopen");
                    break;
                }

                fwrite(shmaddr, 1, strlen(shmaddr), file);
                fclose(file);

                printf("File received and saved at: %s\n", filename);

                receive_file++;

                break;

            case 3:
                // Ngắt kết nối vùng nhớ chia sẻ
                shmdt(shmaddr);

                // Đóng semaphore (tùy chọn)
                sem_close(sem);
                exit(0);

            default:
                printf("Invalid choice. Please enter a valid option.\n");
        }
    }

    return 0;
}
