myshell - Phase 2

myshell은 사용자가 입력한 명령어를 파싱하고 실행하는 간단한 사용자 정의 쉘 프로그램입니다.
Phase 2는 pipeline을 통해 복합 명령어를 모두 실행할 수 있습니다.


구성 파일

csapp.h, csapp,c : 프로그래밍을 위한 유틸리티 함수들이 정의되어 있습니다.
myshell.c : 사용자 정의 쉘의 메인 로직이 구현되어 있습니다.


주요 기능

1. pipeline 명령어 파싱
	문자열을 파싱할 때, '|'과 '&'은 하나의 독립된 명령어로써 argv[]에 저장합니다.
	"\0"은 하나의 명령어 인자로써 argv[][]에 저장합니다.
2. pipeline 명령어 실행
	multiple()를 통해 pipeline을 실행합니다.
	fd[]를 통해 stdin을 저장하고 명령어를 child process에서 실행합니다.
	이후 출력된 stdout을 다시 stdin으로써 fd[]에 저장합니다.
	argv == NULL이 될 때까지 위 과정을 반복합니다.
