#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define REPEATS 10

static volatile sig_atomic_t sig_flag;
static sigset_t newmask, oldmask, zeromask;
static struct sigaction sig_action_buf;

static void sigusr_handler(int signo) {
	sig_flag = 1;
}

void TELL_INIT(void) {
	sig_flag = 0;

	sig_action_buf.sa_handler = sigusr_handler;
	sigemptyset(&sig_action_buf.sa_mask);
	sigaction(SIGUSR1, &sig_action_buf, NULL);

	sigemptyset(&newmask);
	sigemptyset(&zeromask);

	sigaddset(&newmask, SIGUSR1);

	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
}

void NOTIFY(pid_t pid) {
	kill(pid, SIGUSR1);
}

void WAIT_NOTIFY(void) {
	while (sig_flag == 0) {
		sigsuspend(&zeromask);
	}

	sig_flag = 0;
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}

static char msg_buff[] = "A message from a process.\n";

static void write_msgs(const char *proc) {
	int i;
	for (i = 0; i < REPEATS; i++) {
		write(STDOUT_FILENO, proc, strlen(proc));
		write(STDOUT_FILENO, msg_buff, sizeof(msg_buff)-1);
		sleep(1);
	}
}

int main(void) {
	printf("Started\n");
	TELL_INIT();

	pid_t f = fork();
	if (f < 0) {
		perror("Couldn't fork");
		exit(EXIT_FAILURE);
	}

	if (f == 0) {
		write_msgs("child: ");
		NOTIFY(getppid());
		/* Now we wait for the parent to tell us it's time to exit */
		WAIT_NOTIFY();
	} else {
		/* Child goes first */
		WAIT_NOTIFY();
		write_msgs("parent: ");

		printf("Press any key to terminate\n");
		getchar();
		NOTIFY(f);
	}

	return 0;
}