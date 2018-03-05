#include <stdlib.h>
#include <string.h>
#include "uv.h"
#include "sqlite3.h"
#include "log.h"

// taken from learnuv
#define CHECK(r, msg) if (r) {                                                       \
  log_error("%s: [%s(%d): %s]\n", msg, uv_err_name((r)), (int) r, uv_strerror((r))); \
  exit(1);                                                                           \
}

#define CHECK_SQL(r, db, msg) if (r) {                                             \
  log_error("%s: [SQLite3 Error (%d): %s]\n", msg, (int) r, sqlite3_errmsg((db))); \
  exit(1);                                                                         \
}

struct query_req {
  sqlite3* db;
  const char* query;
};

void start_query_work(uv_work_t* worker) {
  sqlite3_stmt* stmt;
}

void finish_query_work(uv_work_t* worker, int status) {

}

void run_query(uv_loop_t* loop, sqlite3* db, const char* query) {
  int r = 0;
  uv_work_t* worker = malloc(sizeof(uv_work_t));
  struct query_req* qr = malloc(sizeof(struct query_req));

  qr->db = db;
  qr->query = query;

  worker->data = qr;
  r = uv_queue_work(loop, worker, start_query_work, finish_query_work);
  CHECK(r, "uv_queue_work");
}

int main() {
  int r = 0;
  uv_loop_t* loop = uv_default_loop();
  sqlite3* db;
  sqlite3_stmt* stmt;
  uv_work_t* stmt_worker = malloc(sizeof(uv_work_t));

  const char* query = \
    "create table if not exists tab ("
    "id int"
    ");";

  r = sqlite3_open("awesome_db", &db);
  CHECK_SQL(r, db, "sqlite3_open");

  r = sqlite3_prepare(db, query, strlen(query), &stmt, &query);
  CHECK_SQL(r, db, "sqlite3_prepare");

  r = sqlite3_step(stmt);
  log_info("Statement step: %d", r);

  r = sqlite3_column_type(stmt, 0);
  log_info("Result column type: %d", r);

  r = sqlite3_step(stmt);
  log_info("Statement step: %d", r);

  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}
