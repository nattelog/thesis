#include <stdio.h>
#include <uv.h>

int64_t counter = 0;

// this callback is called every time step 5 is reached
void wait_for_a_while(uv_idle_t* handle) {
  counter++;

  // when the counter is large enough, the handle is closed and the loop will
  // die
  if (counter >= 10e6)
    uv_idle_stop(handle);
}

int main() {
  uv_idle_t idler;

  uv_idle_init(uv_default_loop(), &idler);
  uv_idle_start(&idler, wait_for_a_while);

  printf("Idling...\n");

  // this function will not return until the loop is no longer alive
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  uv_loop_close(uv_default_loop());
  return 0;
}
