# Operating System Project 1

| StudentID | Name               |
|-----------|--------------------|
| 20205035  | Yeonhyuk Kim (김연혁) |

## Problem definition

![original_puml](https://cdn-0.plantuml.com/plantuml/png/ZLHDRzim3BthLn2zr46JvXvq0mPiM7l9OUWEEmcgnSR2PCcJg6RzzwFaZoIkk-tYMERZ8zyJcUk6g8kxxRQpob5gxMtjsst4NmbTZHjjr26eLeXE1JArwHW4LsHQ3BVr5QW8vNqMRW8gFSRBuH9W7648gQC1Cvs5K8cyI-qJSuhDnvvz1y_1jn0jOeTX3RMoje_8ygrYvfzn9SedE6MTuGgYUWjHCM8WCC_mCFR0xN3NAJZKu9qGrliEZesnE72XLLtaO1M9HQtW3Wgx03Yn5z5mSpCsKSxorQ3Lt0MCW0dAk1JWmmZaGq17C-Xaqy3Avo2rYUh9OhLQGoGV4BpLiaGilg0LpA1iKJYGZeIiqdaEQ8jok2I0fSww59jvCdd8lw_IcaYSXqxdCilA6kKEE6TWymWpp-Fzjo-VFily-B6GVp8nrWtgbAMAvJePvF0t3N8aUHcIgrQCG0S7d6gNSRhedvr02ERGZ9ZYHJIy0mk-HTLYdjkpOdFETI4u8cXVREuL70tLpP0Ho-rcutBn9KdpyHaDaAICVM-C_i_ix_lzm_xhMT6xpKIssR6El03_hBkOzDurBKvlHEhdjBeSspwthDFVEYiB9KGsCQ-f41dSR_O8FFrDY3y0)

위는 기존 timer 가 구현되어있는 내용을 시퀀스 다이어그램으로 작성한 것이다.

`idle_thread` 가 아닌 경우 스레드(프로세스)는 `ready_list` 에 추가되고(7), THREAD_READY 상태로 변경된다(8).

이후 `schedule()` 이 호출되면(9) `ready_list` 에 들어있던 스레드를 꺼내 현재 스레드와 비교하는데,
(10 아래 노트) 동일하기 때문에 `switch_threads()` 를 실행하지 않고 `thread_schedule_tail()` 이 실행되어(11)
thread 의 상태는 다시 `THREAD_RUNNING` 으로 변경되고(12) `thread_yield()` 가 리턴된다(16).

해당 스레드는 다시 loop 을 돌며 위 로직을 반복하게 되며, **sleep 하는 동안 다른 스레드가 실행되지 않는다**.

위와 같은 이유 때문에 스레드가 일을 하지 않는 sleep 상태가 되어도 `THREAD_READY` 와 `THREAD_RUNNING` 만 반복하여 변경되기 때문에 cpu 점유
상태가 지속되고 다른 스레드의 실행이 불가능해진다.

![obj3](For_PintOS-2026-03-17-20-05-39%20-%20obj3.png)

pintos 에서 실행을 해보았을 때에도 위와 같이 kernel thread (sleep 이 실행된 스레드) 가 cpu 를 계속 점유하고 있어 kernel ticks 만
늘어난 것을 확인할 수 있다.

## Policy and algorithm design

![reimplemented_puml](https://cdn-0.plantuml.com/plantuml/png/VLJBRjim4BppAnQvn8QZkBqQKO0s2T2Y1GeKlVHaqDBQ8aoHAXyjylSbLwAibws3BPApmzdTMTrNpZLsjzqsaYldQ_bOw7fhyOz7LM1M2duol6QiuSQ9GZG117TEr6WUYplW5kXzSbmPv6KwfyKO89H3OtpZEao_PenxfvMl3u6JPSyj-mwEHjTW9MA3vW4ABcLxigUzLM0-spMZBRX43N24MCqXt5eq3iGHlYODGKvGxOt24hHYWn3A9D7IehlVAoTa8AUz8RGKbXya3XASOwpJ1e4emEasopqjhu8BRmo6GdRdAKgeqxxJ9OLTkcie_I1rSGubKTeX676g4XyR0g3cPqnTZBtwzUNdswVN_UVlFrw-lRq2LsNBQeiAIoynC1xUQIl86Um-W7PLA6JIN747rOKl_loEbJTUgH39C117QOpbEGH6KKhiieCydtgZB_387VtkGkV94R39uUFj3aVSABoLiDcCOVz2cNplHAbr0mwEsa34eCGwzakevKP1UBWa6xhiUU00UCfReGI9pwnREr28v84e0Nn3BV6AkjU7pQqmkIY73pV01clz5sD6_I0kvR0KFAjK9stbrjUJch2bc6jc9FCCOb11ustLckmo9F7_VQosTrbQX3QGzM87qm0zPNrMMQn5_3XCb7N-k8Li7S8Yf7LWEXvRbVpEZwuVbRvWwpWUiP984p2vSNfT-sLQNxil7Vy7)

위는 문제를 해결하기 위해서 다시 구현할 `timer.c` 와 `thread.c` 의 시퀀스 다이어그램을 작성한 것이다.

어떤 스레드에서 `timer_sleep` 이 호출되면 (1) 이전처럼 `thread_yield()` 가 호출되어 cpu 를 점유하고 있는 것이 아니라 **blocking** 상태로 두어
cpu 가 해당 스레드를 실행하지 않게 하고 다른 스레드로의 cpu scheduling 을 가능하게 할 것이다.

그러기 위해 `thread.c` 에 sleep 중인 스레드를 보관하는 리스트를 추가, `timer_sleep` 이 호출되면 이 스레드를 해당 리스트에 추가할 것이다.

`timer_sleep` 이 호출되면 (1) `timer.c` 에서는 새로운 메소드 `thread_sleep()` 을 호출한다 (4).
`thread_sleep` 메소드에서는 해당 스레드가 interrupt 되지 않게 막고 (5) sleep_until (어느 tick 까지 sleep 할지) 을 `thread` 구조체에 저장 (6),
`thread_block()` 을 호출한다 (8). thread_block 메소드에서 해당 스레드는 THREAD_BLOCKED 로 상태가 변경되고 `schedule()` 을 호출한다.
`schedule` 메소드에서 `ready_list` 에 들어있는 다른 스레드가 꺼내져서 해당 스레드로의 스위칭이 일어나게 된다.

그렇게 스레드의 스위칭이 일어나는 와중에 `sleeper` 실행 전 만들어서 실행되던 `idle_thread` 에서는 `timer_sleep` 메소드의 호출이 일어나지 않았고,
대신 interrupt 가 걸리며 `timer_interrupt` 메소드가 호출된다(9) (다른 스레드도 마찬가지로 interrupt 가 걸리면 `timer_interrupt` 메소드는
호출되고, `thread_tick` 이 호출된다). `idle_thread` 스레드에서 `thread_tick` 이 호출되어 (11) idle_tick 카운트가 1 늘어나게 되고,
이후 새로 추가한 `thread_wakeup()` 을 호출한다 (12). `thread_wakeup` 메소드에서 이전에 저장한 `sleeping_thread_list` 를 모두 순회하며
리스트에 들어있는, 잠들어있는 스레드가 깨어날 때가 되었는지 체크하고 (12 뒤 loop, alt) 스레드가 깨어날 때가 되었다면 해당 스레드를
`sleeping_thread_list` 에서 제거 (14), `thread_unblock()` 을 호출하여 (15) 스레드의 상태를 THREAD_READY 로 변경하고
`ready_list` 에 집어넣어 추후 schedule 시 해당 스레드가 실행되게 한다. 그러고 나면 이후 `schedule` 이 되어 sleep 을 시작할 시 호출되었던
`thread_sleep` 이 리턴되고 (19), `timer_sleep` 이 리턴되게 된다 (20).

## Mechanism (implementation)

이번 프로젝트에서 새로 추가된 메소드는 `thread.c` 에 `thread_sleep`, 그리고 `thread_wakeup` 이며, 수정된 메소드는
`timer.c` 의 `timer_sleep`, `timer_interrupt`, `thread.c` 의 `thread_init` 메소드이다.
(`thread.c` 의 메소드를 외부에서 호출 할 수 있도록 `thread.h` 에 메소드 declaration)

추가로 sleep 중인 스레드를 저장하는 list 또한 thread.c 에 static 변수로 추가하였다.

```c++
// thread.c

static struct list sleeping_thread_list;

// ...

void
thread_init (void)
{
  //...
  list_init(&sleeping_thread_list); // Added for list initialization
    
  //..
}
```

thread_sleep 과 thread_wakeup 메소드는 위 sequence diagram 대로 구현되었다. thread_sleep 메소드의 argument sleep_until 은
해당 스레드가 어느 tick 까지 sleep 하고 있을지를 저장하고 있고, thread_wakeup 메소드의 argument tick 은 현재 cpu tick 을 저장하고 있다.

```c++
// thread.c
void thread_sleep (int64_t sleep_until) {
    enum intr_level old_level;
    struct thread *cur = thread_current();
    ASSERT (cur != idle_thread);

    old_level = intr_disable ();

    cur->sleep_until = sleep_until;
    list_push_back(&sleeping_thread_list, &cur->elem);
    thread_block();

    intr_set_level (old_level);
}

void thread_wakeup (int64_t tick) {
    struct thread *t;
    struct list_elem *e = list_begin(&sleeping_thread_list);

    while (e != list_end(&sleeping_thread_list)) {
        t = list_entry (e, struct thread, elem);
        if (t->sleep_until <= tick) {
            // wakeup!
            t->sleep_until = 0;
            e = list_remove(e);
            thread_unblock (t);
        } else {
            e = list_next(e);
        }
    }
}
```

timer.c 에서는 timer_sleep 메소드 호출 시 thread.c 의 thread_yield 가 아니라 thread_sleep 을 호출하게 되어있고,
timer_interrupt 메소드 호출 시에는 thread_wakeup 을 추가로 호출하게 되어있다.

```c++
// timer.c
void
timer_sleep (int64_t ticks) 
{
  int64_t start = timer_ticks ();

  ASSERT (intr_get_level () == INTR_ON);

  thread_sleep(start + ticks); // Added for thread sleep
}

static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  thread_wakeup(ticks); // Added for thread wakeup
}
```

변경사항만을 갖고 있는 github PR 링크는 다음과 같다. [PR](https://github.com/BranKein/ec4205_OS_PintOS_Project/pull/1/changes)
(docs 폴더 제외하고 확인하시면 됩니다.)

![obj4](For_PintOS-2026-03-18-01-11-31%20-%20obj4.png)

위 내용을 반영하여 pintos 에서 실행을 해보면 위와 같이 kernel ticks 는 단 31번만 카운팅 되었고, 모든 스레드가 blocking 되어
실행이 되지 않는 동안에는 idle_thread 만이 ready_list 에 존재하여 idle_ticks 가 대부분 카운팅 된 것을 볼 수 있다. 