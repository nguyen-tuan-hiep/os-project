#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_SIZE 1024  // Kích thước của vùng nhớ chia sẻ
#define SEM_NAME "/my_semaphore"

int main(int argc, char *argv[]) {
    int shmid;
    key_t key;
    char *shmaddr;
    sem_t *sem;

    // Tạo hoặc truy cập vùng nhớ chia sẻ
    key = ftok(".", 'S');  // Sử dụng chung key như sender
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
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

    while (1) {
        printf("Menu:\n");
        printf("1. Send a message\n");
        printf("2. Send a file\n");
        printf("3. Quit\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);
        getchar(); // Consume newline character from previous input

        switch (choice) {
            case 1:
                printf("Enter a message to send: ");
                fgets(shmaddr, SHM_SIZE, stdin);

                // Báo hiệu cho receiver rằng thông điệp đã sẵn sàng
                sem_post(sem);
                printf("Message sent: %s", shmaddr);
                break;

            case 2: {
                printf("Enter the path to the file to send: ");
                char input_buffer[SHM_SIZE];
                fgets(input_buffer, SHM_SIZE, stdin);
                input_buffer[strcspn(input_buffer, "\n")] = 0; // Loại bỏ ký tự newline

                // Mở tệp tin để đọc và gửi nội dung vào shared memory
                FILE *file = fopen(input_buffer, "rb");
                if (file == NULL) {
                    printf("File not found.\n");
                    perror("fopen");
                    break;
                }

                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);

                if (file_size > SHM_SIZE) {
                    printf("File size exceeds the shared memory size.\n");
                    fclose(file);
                    break;
                }

                fread(shmaddr, 1, file_size, file);
                fclose(file);

                // Báo hiệu cho receiver rằng tệp tin đã sẵn sàng
                sem_post(sem);
                printf("File sent: %s\n", input_buffer);
                break;
            }

            case 3:
                // Ngắt kết nối vùng nhớ chia sẻ
                shmdt(shmaddr);

                // Xóa vùng nhớ chia sẻ (tùy chọn)
                shmctl(shmid, IPC_RMID, NULL);

                // Đóng semaphore (tùy chọn)
                sem_close(sem);
                exit(0);

            default:
                printf("Invalid choice. Please enter a valid option.\n");
        }
    }

    return 0;
}
