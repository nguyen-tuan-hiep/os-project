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
    key = ftok(".", 'S');  // Tạo key IPC từ tên file và một ký tự S
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) == -1) {  // IPC_CREAT: Tạo vùng nhớ chia sẻ nếu chưa tồn tại, 0666: Quyền truy cập vùng nhớ chia sẻ
        perror("shmget");
        exit(1);
    }

    // Kết nối vùng nhớ chia sẻ vào không gian bộ nhớ của tiến trình
    shmaddr = shmat(shmid, NULL, 0);    //Map vùng nhớ chia sẻ vào không gian bộ nhớ của tiến trình
    if (shmaddr == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // Tạo semaphore hoặc sử dụng semaphore đã tạo
    sem = sem_open(SEM_NAME, O_RDWR | O_CREAT, 0666, 1);
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
                fgets(shmaddr, SHM_SIZE, stdin);    // Đọc chuỗi từ stdin và lưu vào vùng nhớ chia sẻ
                sem_post(sem);  // Báo hiệu (signal) qua semaphore để thông báo cho receiver biết rằng có thông điệp mới.
                printf("Message sent: %s\n", shmaddr);
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

                fseek(file, 0, SEEK_END);   // Đưa con trỏ tệp tin về cuối tệp tin để tính kích thước
                long file_size = ftell(file);   // Lấy kích thước file
                fseek(file, 0, SEEK_SET);

                if (file_size > SHM_SIZE) {
                    printf("File size exceeds the shared memory size.\n\n");
                    fclose(file);
                    break;
                }

                fread(shmaddr, 1, file_size, file);     // Đọc nội dung tệp tin và lưu vào vùng nhớ chia sẻ
                fclose(file);

                // Báo hiệu (signal) cho receiver rằng tệp tin đã sẵn sàng
                sem_post(sem);
                printf("File sent: %s\n\n", input_buffer);
                break;
            }

            case 3:
                // Ngắt kết nối vùng nhớ chia sẻ
                shmdt(shmaddr);

                // Xóa vùng nhớ chia sẻ (optional)
                shmctl(shmid, IPC_RMID, NULL);

                // Đóng semaphore
                sem_close(sem);
                exit(0);

            default:
                printf("Invalid choice. Please enter a valid option.\n");
        }
    }
    sem_close(sem);
    return 0;
}
