#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct Address {
    int id;
    int set;
    char *name;
    char *email;
};

struct Database {
    int max_rows;
    int max_data;
    struct Address *rows;
};

struct Connection {
    FILE *file;
    struct Database *db;
};

void die(const char *message) {
    if (errno) {
        perror(message);
    } else {
        printf("ERROR: %s\n", message);
    }
    exit(1);
}

void Address_print(struct Address *addr) {
    printf("%d %s %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn) {
    // 1. 读取数据库的配置参数
    if (fread(&conn->db->max_rows, sizeof(int), 1, conn->file) != 1) die("Failed to load max_rows.");
    if (fread(&conn->db->max_data, sizeof(int), 1, conn->file) != 1) die("Failed to load max_data.");

    // 2. 为行数组分配内存
    conn->db->rows = malloc(sizeof(struct Address) * conn->db->max_rows);
    if (!conn->db->rows) die("Memory error: failed to allocate rows.");

    // 3. 逐行读取数据并分配字符串内存
    for (int i = 0; i < conn->db->max_rows; i++) {
        struct Address *row = &conn->db->rows[i];
        if (fread(&row->id, sizeof(int), 1, conn->file) != 1) die("Failed to load id.");
        if (fread(&row->set, sizeof(int), 1, conn->file) != 1) die("Failed to load set status.");
        
        row->name = malloc(conn->db->max_data);
        row->email = malloc(conn->db->max_data);
        
        if (fread(row->name, conn->db->max_data, 1, conn->file) != 1) die("Failed to load name.");
        if (fread(row->email, conn->db->max_data, 1, conn->file) != 1) die("Failed to load email.");
    }
}

struct Connection *Database_open(const char *filename, char mode) {
    struct Connection *conn = malloc(sizeof(struct Connection));
    if (!conn) die("Memory error");

    conn->db = malloc(sizeof(struct Database));
    if (!conn->db) die("Memory error");

    if (mode == 'c') {
        conn->file = fopen(filename, "w");
    } else {
        conn->file = fopen(filename, "r+");
        if (conn->file) {
            Database_load(conn);
        }
    }

    if (!conn->file) die("Failed to open the file");

    return conn;
}

void Database_close(struct Connection *conn) {
    if (conn) {
        if (conn->file) fclose(conn->file);
        if (conn->db) {
            if (conn->db->rows) {
                // 逐行释放 name 和 email
                for (int i = 0; i < conn->db->max_rows; i++) {
                    struct Address *row = &conn->db->rows[i];
                    if (row->name) free(row->name);
                    if (row->email) free(row->email);
                }
                free(conn->db->rows); // 释放行数组
            }
            free(conn->db); // 释放数据库结构体
        }
        free(conn); // 释放连接结构体
    }
}

void Database_write(struct Connection *conn) {
    rewind(conn->file);

    // 1. 写入配置参数
    if (fwrite(&conn->db->max_rows, sizeof(int), 1, conn->file) != 1) die("Failed to write max_rows.");
    if (fwrite(&conn->db->max_data, sizeof(int), 1, conn->file) != 1) die("Failed to write max_data.");

    // 2. 逐行写入数据
    for (int i = 0; i < conn->db->max_rows; i++) {
        struct Address *row = &conn->db->rows[i];
        if (fwrite(&row->id, sizeof(int), 1, conn->file) != 1) die("Failed to write id.");
        if (fwrite(&row->set, sizeof(int), 1, conn->file) != 1) die("Failed to write set status.");
        if (fwrite(row->name, conn->db->max_data, 1, conn->file) != 1) die("Failed to write name.");
        if (fwrite(row->email, conn->db->max_data, 1, conn->file) != 1) die("Failed to write email.");
    }

    fflush(conn->file);
}

void Database_create(struct Connection *conn, int max_rows, int max_data) {
    conn->db->max_rows = max_rows;
    conn->db->max_data = max_data;
    conn->db->rows = malloc(sizeof(struct Address) * max_rows);
    if (!conn->db->rows) die("Memory error");

    for (int i = 0; i < max_rows; i++) {
        // 初始化每个地址
        conn->db->rows[i].id = i;
        conn->db->rows[i].set = 0;
        conn->db->rows[i].name = malloc(max_data);
        conn->db->rows[i].email = malloc(max_data);
        
        // 使用 memset 清空字符串内存
        memset(conn->db->rows[i].name, 0, max_data);
        memset(conn->db->rows[i].email, 0, max_data);
    }
}

void Database_set(struct Connection *conn, int id, const char *name, const char *email) {
    struct Address *addr = &conn->db->rows[id];
    if (addr->set) die("Already set, delete it first");

    addr->set = 1;
    
    // 使用 strncpy 确保不会溢出，并手动确保以 \0 结尾
    strncpy(addr->name, name, conn->db->max_data - 1);
    addr->name[conn->db->max_data - 1] = '\0';

    strncpy(addr->email, email, conn->db->max_data - 1);
    addr->email[conn->db->max_data - 1] = '\0';
}

void Database_get(struct Connection *conn, int id) {
    struct Address *addr = &conn->db->rows[id];
    if (addr->set) {
        Address_print(addr);
    } else {
        die("ID is not set");
    }
}

void Database_delete(struct Connection *conn, int id) {
    struct Address *addr = &conn->db->rows[id];
    addr->set = 0;
    // 重置字符串
    memset(addr->name, 0, conn->db->max_data);
    memset(addr->email, 0, conn->db->max_data);
}

void Database_list(struct Connection *conn) {
    int i = 0;
    struct Database *db = conn->db;

    for (i = 0; i < db->max_rows; i++) {
        struct Address *cur = &db->rows[i];
        if (cur->set) {
            Address_print(cur);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) die("USAGE: ex17 <dbfile> <action> [action params]");

    char *filename = argv[1];
    char action = argv[2][0];
    struct Connection *conn = Database_open(filename, action);
    int id = 0;

    // 非 c (create) 操作才需要 id 参数
    if (action != 'c' && argc > 3) {
        id = atoi(argv[3]);
        if (id >= conn->db->max_rows) die("There's not that many records.");
    }

    switch (action) {
        case 'c':
            if (argc != 5) die("Need max_rows and max_data. USAGE: ex17 <dbfile> c <max_rows> <max_data>");
            int max_rows = atoi(argv[3]);
            int max_data = atoi(argv[4]);
            if (max_rows <= 0 || max_data <= 0) die("max_rows and max_data must be greater than 0");
            
            Database_create(conn, max_rows, max_data);
            Database_write(conn);
            break;

        case 'g':
            if (argc != 4) die("Need an id to get");
            Database_get(conn, id);
            break;

        case 's':
            if (argc != 6) die("Need id, name, email to set");
            Database_set(conn, id, argv[4], argv[5]);
            Database_write(conn);
            break;

        case 'd':
            if (argc != 4) die("Need id to delete");
            Database_delete(conn, id);
            Database_write(conn);
            break;

        case 'l':
            Database_list(conn);
            break;

        default:
            die("Invalid action, only: c=create, g=get, s=set, d=del, l=list");
    }

    Database_close(conn);
    return 0;
}