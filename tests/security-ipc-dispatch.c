/* IPC callback fields are distinct members, never an array. */
#define FIO_IPC
#include "test-helpers.h"

static unsigned calls[3];
static int cookie;

static void on_call(fio_ipc_s *ipc) {
  FIO_ASSERT(ipc->udata == &cookie, "call udata");
  ++calls[0];
}
static void on_reply(fio_ipc_s *ipc) {
  FIO_ASSERT(ipc->udata == &cookie, "reply udata");
  ++calls[1];
}
static void on_done(fio_ipc_s *ipc) {
  FIO_ASSERT(ipc->udata == &cookie, "done udata");
  ++calls[2];
}

int main(void) {
  FIO_ASSERT(!fio_ipc_opcode_register(.opcode = 0x53454355,
                                       .call = on_call,
                                       .on_reply = on_reply,
                                       .on_done = on_done,
                                       .udata = &cookie),
             "register opcode");
  const uint16_t flags[] = {0,
                            FIO_IPC_FLAG_REPLY,
                            FIO_IPC_FLAG_DONE,
                            FIO_IPC_FLAG_REPLY | FIO_IPC_FLAG_DONE};
  for (size_t opcode = 0; opcode < 2; ++opcode) {
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i) {
      fio_ipc_s *ipc = fio_ipc_new(.call = on_call,
                                   .on_reply = on_reply,
                                   .on_done = on_done,
                                   .udata = &cookie);
      FIO_ASSERT(ipc, "allocate dispatch message");
      ipc->routing_flags = flags[i];
      if (opcode) {
        ipc->opcode = 0x53454355;
        ipc->udata = NULL; /* replaced by the opcode's registered value */
        fio___ipc_opcode_task(ipc, NULL);
      } else {
        fio___ipc_direct_task(ipc, NULL);
      }
    }
  }
  FIO_ASSERT(calls[0] == 2 && calls[1] == 4 && calls[2] == 2,
             "callback routing changed: %u/%u/%u",
             calls[0], calls[1], calls[2]);
  fio_queue_perform_all(fio_io_queue());
  fprintf(stderr, "\t* IPC typed callback dispatch checks passed\n");
  return 0;
}
