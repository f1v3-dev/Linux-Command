#include <stdio.h>			// printf(), gets()
#include <string.h>			// strcmp(), strlen()
#include <stdlib.h>			// exit()
#include <unistd.h>			// write(), close(), unlink(), read(), write()
#include <fcntl.h>			// open() option

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
	/* TODO: 여기에 fifo file 생성하는 코드 삽입할 것 (이건 다음 시간에 할 에정임) */

	// 클라이언트 -> 서버 방향 통신용 FIFO 연결: 클라이언트가 접속하길 대기함

	/* TODO: 여기에 클라이언트 -> 서버 방향 통신용 FIFO 연결용 코드를 삽입할 것 */
	in_fd = open(c_to_s, O_RDONLY);
	if (in_fd < 0)
		print_err_exit(c_to_s);
	// 서버 -> 클라이언트 방향 통신용 FIFO 연결

	/* TODO: 여기에 서버 -> 클라이언트 방향 통신용 FIFO 연결용 코드를 삽입할 것 */
	out_fd = open(s_to_c, O_WRONLY);
	if (out_fd < 0)
		print_err_exit(s_to_c);
}

// ****************************************************************
// 클라이언트와 연결을 해제한다.
// ****************************************************************
void
dis_connect()
{
	close(out_fd);
	close(in_fd);
}

void duplicate_IO(){
	// 서버의 표준 입력을 수신용 FIFO 파일 핸들로 변경하고
	// 표준 출력, 에러출력을 송신용 FIFO 파일 핸들로 변경한다.

	dup2(in_fd, 0);		// 0 : STDIN_FILENO		서버 <- 클라이언트
	dup2(out_fd, 1);	// 1 : STDOUT_FILENO	서버 -> 클라이언트
	dup2(out_fd, 2);	// 2 : STDERR_FILENO	서버 -> 클라이언트

	// 표준 입출력 버퍼를 없앤다. 입출력이 중간에 버퍼링 되지 않고 바로 I/O됨
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	// stderr는 기본적으로 버퍼링 되지 않음

	// 표준 입출력 0, 1, 2 핸들에 복제 되었으므로 이제 in_fd, out_fd는 필요 없음
	dis_connect();	// close(in_fd); close(out_fd);
	// 표준 I/O 핸들 0, 1, 2를 통해 클라이언트와 연결되어 있으므로
	// close()해도 실제로 클라이언트와의 전속이 끊어지는 것은 아님

}
// ****************************************************************
// main() 함수
// ****************************************************************
int
main(int argc, char *argv[])
{
	char ret_buf[SZ_STR_BUF];

	connect_to_client(); // 클라이언트 프로그램 cmdc와 연결한다.
	duplicate_IO();

	while (1) {
		// 클라이언트 -> 서버 수신

		/* TODO: 여기에 클라이언트로부터 메시지 수신하는 문장 삽입할 것 */
		len = read(0, cmd_line, SZ_STR_BUF);
		// 클라이언트가 먼저 종료되면 (Ctrl + C)
		// 여기서 len은 0
		if (len <= 0)		// len : 수신된 메세지 길이
			break;
		// 수신된 메시지를 문자열로 만들어 줌
		cmd_line[len] = '\0';

		// cmdc로부터 "exit"를 전달 받으면 서버 프로그램은 여기서 종료
		if (strncmp(cmd_line, "exit", 4) == 0)
			break;

		//printf("%s", cmd_line); // 주석을 풀고 실행해 보고 다시 주석처리하라.
		
		// 서버가 클라이언트에게 보낼 문자열을 작성함: 받은 메시지를 다시 에코해 줌
		sprintf(ret_buf, "server: %s", cmd_line);
		len = strlen(ret_buf);

		/* TODO: 여기에 클라이언트로 메시지 송신하는 문장 삽입할 것 */
		if (write(1, ret_buf, len) != len)
			break;
}
	dis_connect(); // 클라이언트 프로그램과 연결을 해제한다.
}

// 매번 cmdc, srv 프로그램 실행 전에 
// $ ps -u계정이름
// 위 명령어를 실행하여 기존에 실행시켰던 cmdc, srv 프로그램이 있는지 확인하고, 있으면
// $ kill -9 21032(21032는 죽일 프로세스의 process ID)
// 하여 기존 프로그램을 죽인 후 실행해야 한다.
