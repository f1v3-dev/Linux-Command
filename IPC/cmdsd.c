#include <stdio.h>			// printf(), gets()
#include <string.h>			// strcmp(), strlen()
#include <stdlib.h>			// exit()
#include <unistd.h>			// write(), close(), unlink(), read(), write()
#include <fcntl.h>			// open() option
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define SZ_STR_BUF		256

// 메시지 큐용으로 사용될 FIFO 파일
char *s_to_c = "fifo_s_to_c";
char *c_to_s = "fifo_c_to_s";

int  in_fd, out_fd;
int  len;
char cmd_line[SZ_STR_BUF];

// ****************************************************************
// 에러 원인을 화면에 출력하고 프로그램 강제 종료
// ****************************************************************
void 
print_err_exit(char *msg)
{
	perror(msg);
	exit(1);
}

// ****************************************************************
// 클라이언트와 연결한다.
// ****************************************************************

void
connect_to_client()
{
	// 서버와 클라이언트가 통신할 두 개의 FIFO 파일을 생성함.
	// char * c_to_s = "fifo_c_to_s"
	// char * s_to_c = "fifo_s_to_c"
	// 0600 : 파일 접근권한 rw-------
	mkfifo(c_to_s, 0600);	// 클라이언트 -> 서버로 전송할 FIFO 파일
	mkfifo(s_to_c, 0600);	// 서버 -> 클라이언트로 전송할 FIFO 파일
	
	// daemonize() 에서 모든 파일을 close()했기 때문에 open된 파일이 하나도 없음
	// 이 상태에서 파일을 open하면 파일 핸들 값은 0, 1, 2, 3 순서로 할당됨

	// 클라이언트 -> 서버 방향 통신용 FIFO 연결: 클라이언트가 접속하길 대기함
	in_fd = open(c_to_s, O_RDONLY);		// in_fd: 0
	if (in_fd < 0)
		print_err_exit(c_to_s);
	// 서버 -> 클라이언트 방향 통신용 FIFO 연결

	/* TODO: 여기에 서버 -> 클라이언트 방향 통신용 FIFO 연결용 코드를 삽입할 것 */
	out_fd = open(s_to_c, O_WRONLY);	// out_fd: 1
	if (out_fd < 0)
		print_err_exit(s_to_c);

	dup2(out_fd, 2); // out_fd: 1를 핸들 2에 복제함, 따라서 2도 s_to_c를 지칭함
}

// ****************************************************************
// 클라이언트와 연결을 해제한다.
// ****************************************************************
void
dis_connect()
{
	// 클라이언트 cmdc와 연결된 메시지 큐(FIFO 큐)와의 접속을 닫음
	close(0);
	close(1);
	close(2);
}

void daemonize()
{
	int i;
	pid_t pid;
	struct rlimit rl;

	// 파일 접근 모드 매스크를 리셋함. 이 함수를 호출하지 않으면 향후 데몬이
	// 생성할 파일의 접근모드가 creat()에서 지정하는대로 안될 수 있다.
	umask(0);

	// 아래의 setsid()를 호출하는 프로세스는 그룹 리더가 아니어야 한다.
	// 부모가 혹시 그룹리더일 수 있으므로 부모는 죽고 자식이 계속 진행하게 함
	// 부모가 죽었으므로 자식은 절대 그룹리더가 아님
	if ((pid = fork()) < 0)
		print_err_exit("fork");
	else if (pid > 0)			// 부모는 여기서 종료, 자식은 계속 실행
		exit(0);

	// 1) 스스로 세션 리더가 된다.
	// 2) 터미널(키보드와 모니터)과 연결을 해제하고,
	// 3) 세션 내의 그룹의 그룹리더가 됨
	// 지금부터는 키보드 입력과 화면 출력은 안됨
	setsid();
	
	// 터미널(전화선 모뎀)과 연결이 끊어질 때 전달되는 시그널을 무시함
	// 사실 터미널이 없기 때문에 이 신호는 들어오지 않지만,
	// 그래도 혹시 들어오면 무시하게 함
	signal(SIGHUP, SIG_IGN);

	// 향후 본의 아니게 유사(pseudo)터미널 관련 파일(/dev/pts/*)을 open할 경우,
	// 데몬이 세션리더인 경우, 그 파일이 데몬의 터미널로 지정됨(그러면 안됨)
	// 따라서 세션리더인 부모는 죽고 자식이 계속 진행함
	// 세션리더인 부모가 죽었기 때문에 자식은 절대 세션리더가 아님
	if ((pid = fork()) < 0)
		print_err_exit("fork");
	else if (pid > 0)	// 부모는 여기서 종료, 자식은 계속 실행
		exit(0);

	// 지금의 현재 작업 디렉토리가 있는 하드 디스크가 관리자에 의해 시스템에서
	// 동적으로 강제 제거 될 수도 있으므로(고장 등의 이유로) 현재 작업
	// 디렉토리를 /로 변경함 (/는 시스템이 종료되기 전까지는 항상 존재하므로)

	// chdir("/");		// 우리는 현재  디렉토리에서 계속 작업하기로 함

	// 한 프로세스가 open할 수 있는 최대 파일의 수를 얻음
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		print_err_exit("getrlimit: can't get file limit");
	
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;

	// 기존에 open한 파일이 터미널 관련 파일이 있을 수 있으므로
	// 열려있는 모든 파일을 닫는다.
	// 따라서 필요한 파일은 데몬으로 전환한 이후에 open하여야 한다.
	for (i = 0; i < rl.rlim_max; ++i)
		close(i);
}

// ****************************************************************
// main() 함수
// ****************************************************************
int
main(int argc, char *argv[])
{
	daemonize();

	while(1){
		connect_to_client();
		pid_t pid = fork();
		if (pid < 0)
			print_err_exit(" cmdsd: fork()");

		else if (pid == 0)
			execl("../cmd/cmd", "cmd", NULL);
		
		else{
			dis_connect();
			waitpid(pid, NULL, 0);
			sleep(2);
		}
	}
}

// 매번 cmdc, srv 프로그램 실행 전에 
// $ ps -u계정이름
// 위 명령어를 실행하여 기존에 실행시켰던 cmdc, srv 프로그램이 있는지 확인하고, 있으면
// $ kill -9 21032(21032는 죽일 프로세스의 process ID)
// 하여 기존 프로그램을 죽인 후 실행해야 한다.
