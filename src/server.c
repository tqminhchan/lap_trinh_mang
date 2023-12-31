#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int create_listen_socket() {
    
    int listen_socket;
    struct sockaddr_in server_addr;

    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listen_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    if (listen(listen_socket, 10) < 0) {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    return listen_socket;
}

int accept_conn(int listen_socket) {

    int conn_socket;
    struct sockaddr_in client_addr;
    int client_addr_size = sizeof(struct sockaddr);

    if ((conn_socket = accept(listen_socket, (struct sockaddr *) &client_addr, &client_addr_size)) < 0) {
        report_err(ERR_CONN_ACCEPT);
        exit(0);
    }

    return conn_socket;
}

void make_server() {
    
    Account *acc_list;
    int listen_socket;

    acc_list = read_account_list();
    listen_socket = create_listen_socket();

    while (1) {

        int conn_socket = accept_conn(listen_socket);

        Login_req req;
        req.conn_socket = conn_socket;
        req.acc_list = acc_list;

        pthread_t client_thr;
        if (pthread_create(&client_thr, NULL, pre_login_srv, (void *) &req) < 0) {
            report_err(ERR_CREATE_THREAD);
            exit(0);
        }
        pthread_detach(client_thr);
    }

    close(listen_socket);
}

void *pre_login_srv(void *param) {

    Login_req *req = (Login_req *) param;
    Package pkg;

    while (1) {

        recv(req->conn_socket, &pkg, sizeof(pkg), 0);

        switch (pkg.ctrl_signal) {
        case LOGIN_REQ:
            handle_login(req->conn_socket, req->acc_list);
            break;
        case QUIT_REQ:
            printf("user quit\n");
        }

        if (pkg.ctrl_signal == QUIT_REQ) {
            break;
        }
    }
}

void handle_login(int conn_socket, Account *acc_list) {

    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    Package pkg;
    Account *target_acc;
    int result;

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(username, pkg.msg);

    pkg.ctrl_signal = RECV_SUCC;
    send(conn_socket, &pkg, sizeof(pkg), 0);

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(password, pkg.msg);

    printf("%s\n", username);
    printf("%s\n", password);

    target_acc = find_account(acc_list, username);
    if (target_acc) {
        if (target_acc->is_signed_in) {
            result = SIGNED_IN_ACC;
        } else {
            if (strcmp(target_acc->password, password) == 0) {
                result = LOGIN_SUCC;
            } else {
                result = INCORRECT_ACC;
            }
        }
    } else {
        result = INCORRECT_ACC;
    }

    if (result == LOGIN_SUCC) {
        printf("login success\n");
        target_acc->is_signed_in = 1;
    } else if (result == SIGNED_IN_ACC) {
        printf("already signed in acc\n");
    } else {
        printf("incorrect acc\n");
    }

    pkg.ctrl_signal = result;
    send(conn_socket, &pkg, sizeof(pkg), 0);
}

int main() {
    make_server();
    return 0;
}