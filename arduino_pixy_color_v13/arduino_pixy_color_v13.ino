/* ==========================================================================
 *
 *      arduino_pixy_color_v13        학 생 용   대 회 버 전
 *
 *      플루토 휴머노이드 - Pixy2 색상 패치 추종 달리기
 *      2026 부천AI로봇경진대회 / 종목03 휴머노이드 스프린트 (AI형)
 *
 * --------------------------------------------------------------------------
 *
 *   [ 대회 당일 순서 ]
 *
 *     1.  로봇 전원을 켠다
 *     2.  로봇을 START 라인에 놓고, 패치 쪽을 보게 한다
 *     3.  화면(또는 로봇)이 "준비 완료" 가 될 때까지
 *         카메라 앞을 손으로 가리고 기다린다
 *     4.  심판이 "시작" 하면  →  손을 뗀다
 *     5.  로봇이 0.1초 안에 출발한다
 *
 *
 * --------------------------------------------------------------------------
 *
 *   [ 로봇이 하는 일 ]
 *
 *       ① 대기     카메라를 가리고 있다가 떼면 출발  (첫 출발만)
 *                     ↓
 *       ② 주행     패치를 보며 걷는다. 빗나가면 서서 방향을 고친다
 *                     ↓
 *       ③ 찾기     패치를 놓치면 제자리에서 돌며 다시 찾는다
 *                     ↓
 *       ④ 마무리   결승선을 지나면 눈감고 조금 더 간다
 *                     ↓
 *       ⑤ 서있기   완주. 서 있는다.
 *                  → 학생이 START 로 옮기면 저절로 다시 출발한다
 *
 *       (넘어짐)   로봇이 자이로로 알아서 일어난다.
 *                  아두이노는 그동안 아무 키도 안 보낸다
 *
 *                  ★ v13 : 로봇이 "넘어졌다" 고 알려주지 않아도
 *                    아두이노가 스스로 알아챈다.  ( [넘어짐 감지] 참고 )
 *                      - 카메라 : 가운데 있던 패치가 한순간에 사라지면 넘어짐
 *                      - MPU6050 센서를 달면 기울기로 확실하게 안다
 *                    그래서 넘어지면 "패치 안 보임" 대신 "넘어짐" 으로 나오고,
 *                    일어나는 동안 헛턴(찾기)을 쏘다가 트랙 밖으로
 *                    눈감고 걸어가는 일이 없다.
 *
 * --------------------------------------------------------------------------
 *
 *  카메라값(LENS_K) 을 로봇이 스스로 잰다
 *
 
 *     명령 4개
 *
 *        k       지금 보이는 패치로 재서 저장
 *        +  -    5씩 손보기 (누를 때마다 자동 저장)
 *        x       저장을 지우고 코드에 적힌 값으로 되돌리기
 *
 *     ★ 저장된 값이 코드보다 세다.
 *        코드의 LENS_K 를 고치고 업로드해도 바뀌지 않는다.
 *        저장은 업로드해도 지워지지 않기 때문이다.
 *        코드 값으로 돌아가려면  x  를 눌러야 한다.
 *
 *     켤 때마다 어느 값을 쓰는지 화면에 나온다.
 *        LENS_K : 209  (저장된 값)
 *        LENS_K : 209  (코드 값 - 아직 교정 안 함)
 *
 * --------------------------------------------------------------------------
 
 *
 * --------------------------------------------------------------------------

 * ========================================================================== */

#include <Pixy2.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


/* ##########################################################################
 * ##                                                                      ##
 * ##                 학  생  이     넣  는     숫  자                     ##
 * ##                                                                      ##
 * ##          아래 [1] 하나만 맞으면 대회를 뛸 수 있습니다.               ##
 * ##                                                                      ##
 * ########################################################################## */


/* ──────────────────────────────────────────────────────────────────────────
 *   [1]   색  번  호
 *
 *   무엇인가 :  Pixy2 가 찾을 색이 몇 번인지.
 *
 *   구하는 법:  PixyMon 에서 패치 색을 학습시킨다.
 *               그때 고른 번호가 이 숫자다.  보통 1 번이다.
 *
 *   대회에서 :  색은 빨강 / 초록 / 파랑 중에서 경기 시작 전에 고른다.
 *               → 미리 학습해 둔 색을 고르면 이 숫자를 바꿀 일이 없다.
 *
 *               [보험]  세 색을 모두 1, 2, 3 번에 학습시켜 두면,
 *                       노트북에서 시리얼 모니터에 1 / 2 / 3 을 쳐서
 *                       그 자리에서 바꿀 수 있다. (전원을 끄면 되돌아간다)
 *
 *               [주의]  색은 조명을 심하게 탄다.
 *                       연습실에서 잡히던 색이 대회장에서 안 잡히는 일이 많다.
 *                       노트북과 USB 케이블을 경기 직전까지 꼭 챙겨간다.
 * ────────────────────────────────────────────────────────────────────────── */

#define SIG_NO              1



/* ##########################################################################
 * ##                                                                      ##
 * ##            경  기  장     숫  자     ( 현 장 에 서   수 정 )         ##
 * ##                                                                      ##
 * ##     대회장 크기가 바뀔 수 있다고 규정에 적혀 있다.                   ##
 * ##     현장에서 줄자로 재서 이 세 개만 고치면 된다.                     ##
 * ##                                                                      ##
 * ########################################################################## */


/* ──────────────────────────────────────────────────────────────────────────
 *
 *                        경 기 장 을   위 에 서   본   그 림
 *
 *        출발선                                결승선           패치
 *          │                                      │              ▓▓
 *   ┌──────┼──────────────────────────────────────┼──────────┐   ▓▓
 *   │START │                                      │  FINISH  │   ▓▓
 *   └──────┼──────────────────────────────────────┼──────────┘   ▓▓
 *          │                                      │              ▓▓
 *          ├────────────  TRACK_MM  ──────────────┤              ▓▓
 *          │              (2000)                  │              ▓▓
 *          │                                      │              ▓▓
 *          │                                      ├─ FINISH_MM ──┤
 *          │                                      │    (760)     │
 *          │                                      │              │
 *          │                                      ├─ CROSS_MM ─┤ │
 *          │                                      │   (450)    │ │
 *          │                                      │            ↑ │
 *          │                                      │       여기서 선다
 *
 * ────────────────────────────────────────────────────────────────────────── */


/* ──────────────────────────────────────────────────────────────────────────
 *   [2]   T R A C K _ M M      출발선 ~ 결승선 거리
 *
 *   무엇인가 :  로봇이 실제로 달려야 하는 거리.
 *   구하는 법:  줄자로 출발선에서 결승선까지.
 *   규정 기본값 : 2000
 * ────────────────────────────────────────────────────────────────────────── */

#define TRACK_MM         2000 //출발선~ 결승선 거리 


/* ──────────────────────────────────────────────────────────────────────────
 *   [3]   F I N I S H _ M M    결승선 ~ 색상패치 거리        ★ 중요 ★
 *
 *  ┌────────────────────────────────────────────────────────────────────┐
 *  │  왜 이 숫자가 필요한가                                             │
 *  │                                                                    │
 *  │  카메라는 결승선을 볼 수 없다.  패치까지의 거리만 잰다.            │
 *  │  그래서 로봇에게 이렇게 알려줘야 한다.                             │
 *  │                                                                    │
 *  │      " 패치까지 ○○ mm 남았으면, 그 자리가 결승선이다 "            │
 *  │                                                                    │
 *  │  그 ○○ 이 이 숫자다.                                              │
 *  └────────────────────────────────────────────────────────────────────┘
 *
 *   구하는 법:  줄자로  결승선 → 패치 앞면  까지 잰다.
 *
 *   규정 기본값 : 760      ( 경기장 FINISH 구역 길이 )
 *
 *
 *   ── 잘못 넣으면 어떻게 되는가 ──────────────────────────────────────
 *
 *     넣은 값        카메라가 꺼지는 곳        최종 정지 위치      결과
 *    ─────────────────────────────────────────────────────────────────
 *     너무 작다      결승선을 460mm 지나서     결승선 뒤 910mm    경기장
 *      (300)         (아직 카메라로 감)                           밖으로 나감
 *
 *     딱 맞다        결승선 위                 결승선 뒤 450mm    여유 310mm
 *      (760)                                                      안전
 *
 *     너무 크다      결승선 440mm 앞           결승선 뒤 450mm    눈감고 가는
 *      (1200)                                                     거리가 길어짐
 *
 *
 *   ── 꼭 지켜야 할 식 ────────────────────────────────────────────────
 *
 *        FINISH_MM  -  CROSS_MM   ≥   300 mm
 *
 *        760 - 450 = 310   ✅  패치까지 310mm 남으니 안 부딪힌다
 *        300 - 450 = -150  ❌  패치를 150mm 지나쳐 부딪힌다
 *
 *        ※ 이 식은 아래에서 컴파일할 때 자동으로 검사한다.
 *           어기면 업로드 자체가 막힌다.
 * ────────────────────────────────────────────────────────────────────────── */

#define FINISH_MM         760 //거리1


/* ──────────────────────────────────────────────────────────────────────────
 *   [4]   C R O S S _ M M      결승선을 넘어 더 갈 거리
 *
 *   무엇인가 :  결승선에 도착한 뒤 눈감고 더 걸어갈 거리.
 *               확실히 결승선을 통과시키기 위한 여유다.
 *
 *   크게 하면 : 확실히 통과하지만 패치에 가까워진다.
 *   작게 하면 : 결승선을 아슬아슬하게 못 넘을 수 있다.
 *
 *   기본값 : 450
 * ────────────────────────────────────────────────────────────────────────── */

#define CROSS_MM         50 //거리2 설명문 거리1 값에서 여유 300 주고 설정 하면됨 300 넘으면 업로드 안됨 760에서 사용 가능한 범위 50~460 


/* ──────────────────────────────────────────────────────────────────────────
 *   [5]   P A T C H _ M M      색상 패치의 가로 폭
 *
 *   규정 기본값 : 400   (400 x 400 mm)
 *   패치 크기가 다른 대회에 나가면 이 한 줄만 고치면 된다.
 * ────────────────────────────────────────────────────────────────────────── */

#define PATCH_MM          400 //패치 크기



/* ##########################################################################
 * ##                                                                      ##
 * ##                   로  봇  에    맞  추  는    숫  자                 ##
 * ##                                                                      ##
 * ########################################################################## */


/* ──────────────────────────────────────────────────────────────────────────
 *   [6]   L E N S _ K          카메라값        ★ v12 부터 손댈 일이 없다 ★
 *
 *   무엇인가 :  거리를 재기 위한 카메라 고유값.  로봇마다 다르다.
 *
 *   ┌────────────────────────────────────────────────────────────────────┐
 *   │  v12 에서는 이 숫자를 고칠 필요가 없다.                            │
 *   │                                                                    │
 *   │     로봇을 결승선에 놓고  →  시리얼 모니터에  k  를 친다           │
 *   │     → 로봇이 스스로 재서 아두이노 안에 저장한다                    │
 *   │     → 전원을 꺼도 남는다                                           │
 *   │                                                                    │
 *   │  아래 209 는 "아직 한 번도 안 쟀을 때" 쓰는 임시값일 뿐이다.       │
 *   └────────────────────────────────────────────────────────────────────┘
 *
 *   ── 명령 4개 ───────────────────────────────────────────────────────
 *
 *      k       지금 보이는 패치로 재서 저장
 *      +  -    저장된 값을 5씩 올리고 내리기 (누를 때마다 자동 저장)
 *      x       저장을 지우고 아래 209 로 되돌리기
 *
 *   ── 저장된 값이 항상 이긴다 ────────────────────────────────────────
 *
 *      아두이노에 저장된 값이 있으면 그 값을 쓰고, 없으면 아래 209 를 쓴다.
 *
 *      ★ 아래 209 를 고치고 업로드해도 바뀌지 않는다! ★
 *        저장은 업로드해도 지워지지 않기 때문이다.
 *        아래 값으로 바꾸려면 반드시  x  를 눌러 저장을 지워야 한다.
 *
 *   ── 계산식 (참고) ──────────────────────────────────────────────────
 *
 *          LENS_K  =  폭픽셀 x CAL_DIST_MM / PATCH_MM
 *
 *          예) 760mm 에서 폭이 110 픽셀이면 -> 110 x 760 / 400 = 209
 * ────────────────────────────────────────────────────────────────────────── */

#define LENS_K            209     // 한 번도 안 쟀을 때 쓰는 임시값


/* ──────────────────────────────────────────────────────────────────────────
 *   [6-1]  C A L _ D I S T _ M M    교정할 때 패치를 놓는 거리
 *
 *   k 를 누를 때 "패치가 지금 이만큼 앞에 있다" 고 보고 계산한다.
 *
 *      대회장 : 760  <- 결승선 위에 놓으면 된다. 줄자가 필요 없다
 *      연습실 : 1000 <- 줄자로 1m 를 재서 놓는다
 *
 *   ※ 760 으로 두는 것을 권한다.  거리 계산이 실제로 중요하게 쓰이는 곳은
 *      결승선 판정 하나뿐이라, 바로 그 자리에서 재는 것이 가장 정확하다.
 * ────────────────────────────────────────────────────────────────────────── */

#define CAL_DIST_MM       760 //거리1 k값 인식 할때 실측 거리 


/* ──────────────────────────────────────────────────────────────────────────
 *   [7]   L A N E _ M M        얼마나 빗나가면 방향을 고칠지 (mm)
 *
 *   "이대로 걸으면 패치를 몇 mm 옆으로 스쳐 지나가나" 가 이 값을 넘으면
 *   걸음을 멈추고 제자리에서 방향을 고친다.
 *
 *   레인 반폭이 500mm 이므로 300 정도가 적당하다.
 *   작게 하면 자주 서서 느려지고, 크게 하면 코스를 벗어난다.
 * ────────────────────────────────────────────────────────────────────────── */

#define LANE_MM           600 //-> 턴좌우 범위 


/* ──────────────────────────────────────────────────────────────────────────
 *   [8]   T U R N _ W A I T _ M S    턴을 보내고 기다릴 시간 (ms)
 *
 *   로봇이 한 번 도는 데 걸리는 시간. 이 동안은 카메라를 아예 안 본다.
 *   도는 중에 재면 값이 엉망이 되기 때문이다.
 *
 *   짧으면 : 덜 돈 상태에서 재서 계속 헛돈다
 *   길면   : 느려진다
 * ────────────────────────────────────────────────────────────────────────── */

#define TURN_WAIT_MS      800


/* ──────────────────────────────────────────────────────────────────────────
 *   [9]   W A L K _ S P E E D       걷는 속도 (mm/초)
 *
 *   눈감고 가는 거리를 시간으로 바꿀 때만 쓴다.
 *   줄자로 표시해 두고 10초 걷게 해서 간 거리 / 10 으로 구한다.
 * ────────────────────────────────────────────────────────────────────────── */

#define WALK_SPEED        150



/* ##########################################################################
 * ##                                                                      ##
 * ##            선 생 님 용   -   보 통 은  안  건 드 립 니 다            ##
 * ##                                                                      ##
 * ########################################################################## */

/*  카메라가 좌우로 틀어져 달렸을 때만 쓴다. 보통 0  */
#define CAM_OFFSET          0

/*  ── 모션 키 ──────────────────────────────────────────────────────────
 *  조종기 버튼 번호. 로봇 모션 편집기에서 확인한다.
 *
 *  ★ 규정 5번 : 좌우로 이동하는 "옆 보행" 은 실격이다. (턴은 허용)
 *     아래 두 키가 정말 제자리 회전인지 반드시 눈으로 확인할 것.
 *  ────────────────────────────────────────────────────────────────────── */
 //ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
 //플루토 키값
#define KEY_FORWARD  0x000002     // 전진 (키를 계속 보내야 걷는다)
#define KEY_LEFT     0x008002     // 전진하면서 좌  (한 번 보내면 한 번 돈다)
#define KEY_RIGHT    0x020002   // 전진하면서 우 
#define KEY_STOP     0x001010     // 정지
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
#define USE_STOP_KEY        1     // 1 = 설 때 정지키도 보냄
#define ROBOT_ID            1     // 국번
#define SPEED_OVR         100     // speed override

/*  전진키를 몇 ms 마다 보낼지. 로봇이 계속 걷게 하는 주기  */
#define WALK_KEY_MS       300


/*  ── 로봇이 보내주는 소식 (자이로) ────────────────────────────────────
 *
 *  로봇에 자이로가 있어서 넘어짐을 스스로 판단하고 스스로 일어난다.
 *  문제는 아두이노가 그걸 모른다는 것이다.
 *  모르면 일어나는 8초 동안에도 0.3초마다 전진키를 계속 쏜다.
 *
 *  그래서 로봇이 아두이노에게 한 글자만 알려주면 된다.
 *
 *      로봇 --> 아두이노    "#F"   넘어졌다
 *      로봇 --> 아두이노    "#U"   일어나는 중이다
 *      로봇 --> 아두이노    "#S"   다 일어났다 / 정상이다
 *
 *  아두이노는 #F 나 #U 를 받으면 아무 키도 안 보내고 조용히 기다린다.
 *  #S 를 받으면 방향을 다시 찾고 달리기를 이어간다.
 *
 *  [ 로봇 쪽 만드는 법 ]
 *    - 0.1초마다 같은 글자를 반복해서 보낼 것.
 *      한 번만 보내면 놓칠 수 있다.
 *    - 앞에 '#' 을 꼭 붙일 것. 잡음을 신호로 착각하는 걸 막는다.
 *
 *  [ 알아둘 것 - 통신 속도 ]
 *    115200 은 아두이노가 소프트웨어로 "받기" 에는 빠듯한 속도다.
 *    글자가 깨져 들어올 수 있다. 그래서 반복해서 보내라고 한 것이다.
 *    로봇과 블루투스 속도를 57600 이하로 낮출 수 있으면 훨씬 안정적이다.
 *
 *  [ 로봇이 못 보내주면 ]
 *    USE_ROBOT_FEEDBACK 을 0 으로 두면 된다.  이때는
 *    ★ 로봇 펌웨어가 "일어나는 중에는 들어오는 조종키를 전부 무시" ★
 *    하도록 반드시 만들어야 한다.  안 그러면 일어나다 말고 다시 엎어지는
 *    무한 반복에 빠진다.
 *  ────────────────────────────────────────────────────────────────────── */
#define USE_ROBOT_FEEDBACK  0

#define RX_MARK           '#'     // 신호 앞에 붙는 글자
#define RX_FALLEN         'F'     // 넘어졌다
#define RX_GETUP          'U'     // 일어나는 중
#define RX_OK             'S'     // 정상

/*  넘어졌다는 소식만 오고 일어났다는 소식이 안 올 때,
 *  이 시간이 지나면 아두이노가 알아서 다시 움직인다. (학생이 집어간 경우)  */
#define FALL_MAX_MS    20000UL


/*  ── 넘어짐 감지  (v13 새 기능) ────────────────────────────────────────
 *
 *  [ 왜 필요한가 ]
 *    로봇이 넘어지면 카메라는 바닥이나 천장을 본다 → "패치 안 보임".
 *    v12 는 이걸 "패치를 놓쳤다" 로 착각해서
 *       ① 일어나는 동안 찾기 턴을 계속 쏘고  (로봇은 무시 → 횟수만 소진)
 *       ② 찾기가 다 실패한 걸로 쳐서
 *       ③ 엉뚱한 방향으로 "눈감고 직진" → 트랙 밖으로 나간다.
 *
 *  [ 방법 A ]  카메라로 알아채기     USE_CAM_FALL = 1   (부품 추가 없음)
 *
 *    정상적으로 패치를 놓칠 때는  반드시 화면 "끝" 으로 빠져나간다.
 *    화면 "가운데" 있던 패치가 한순간에 사라지는 것은
 *    카메라가 갑자기 바닥/천장을 향했다는 뜻이다  =  넘어짐.
 *
 *       가운데에서 갑자기 사라짐 → FALL_CONFIRM_MS 동안 계속 안 보임
 *          → "넘어짐" 확정 → 키를 끊고 GETUP_MS 동안 조용히 기다림
 *          → 패치가 보이면 바로 주행, 안 보이면 찾기 (못 찾으면 서서 기다림)
 *
 *    ※ 잠깐 깜빡인 것(조명, 앞을 지나간 손)은 FALL_CONFIRM_MS 안에 다시
 *       보이므로 넘어짐으로 치지 않는다.
 *    ※ 옆으로 쓰러지면 패치가 화면 가운데 그대로 남을 수 있어 못 알아챈다.
 *       그때는 로봇 펌웨어가 일어나는 중 키를 무시해 주어야 한다.
 *
 *  [ 방법 B ]  MPU6050 기울기 센서   USE_IMU_FALL = 1   (가장 확실)
 *
 *    아두이노에 MPU6050 (GY-521) 을 달고 기울기로 직접 판단한다.
 *    앞/뒤/옆 어느 쪽으로 넘어져도, 마무리(눈감고 직진) 중이어도 안다.
 *    일어나서 똑바로 선 순간도 정확히 안다.
 *
 *       MPU6050      아두이노 UNO
 *        VCC   ───   5V
 *        GND   ───   GND
 *        SDA   ───   A4
 *        SCL   ───   A5
 *
 *    출발할 때(손을 뗄 때) 서 있는 자세를 "똑바로" 로 기억한다.
 *    센서를 어떤 방향으로 붙여도 된다. 단, 로봇 몸통에 단단히 고정할 것.
 *
 *  우선순위 :  로봇 소식(#F/#S)  >  MPU6050  >  카메라
 *              위의 것이 켜져 있으면 카메라 짐작은 저절로 꺼진다.
 *  ────────────────────────────────────────────────────────────────────── */
#define USE_CAM_FALL        1     // 1 = 카메라로 넘어짐 짐작
#define USE_IMU_FALL        0     // 1 = MPU6050 으로 넘어짐 감지

/*  ★ 로봇으로 직접 재서 넣기 : 넘어진 순간 ~ 다 일어나 설 때까지 걸리는 시간  */
#define GETUP_MS        9000UL

/*  카메라 짐작에 쓰는 값  */
#define FALL_CENTER_PX     90     // 중심에서 이 안쪽에 있던 패치가 사라지면 의심
#define FALL_GONE_MS      150UL   // 마지막으로 본 지 이 시간 안에 사라져야 "갑자기"
#define FALL_CONFIRM_MS   600UL   // 이만큼 계속 안 보이면 넘어짐으로 확정

/*  MPU6050 에 쓰는 값  */
#define IMU_FALL_DEG       55     // 이만큼 넘게 기울면 넘어짐
#define IMU_UP_DEG         20     // 이 안쪽으로 돌아오면 선 것
#define IMU_FALL_MS       300UL   // 기운 상태가 이만큼 이어져야 넘어짐 (걷다 흔들린 것 거르기)
#define IMU_UP_MS        1500UL   // 선 상태가 이만큼 이어져야 다 일어난 것


/*  ── 패치 찾기 (v11 새 기능) ──────────────────────────────────────────
 *
 *  턴을 몇 번 할지 정해 넣지 않는다.  로봇이 스스로 잰다.
 *
 *   ① 턴을 보내기 직전 패치의 화면 위치를 기억한다
 *   ② 턴이 끝난 뒤 위치를 다시 잰다
 *   ③ 그 차이가 "이 로봇의 턴 한 번 = 화면에서 몇 픽셀" 이다
 *
 *  좌턴과 우턴을 따로 기록한다.  배터리가 닳아 턴이 약해지면
 *  값도 저절로 따라 내려간다.  학생이 넣을 값은 없다.
 *  ────────────────────────────────────────────────────────────────────── */
//──────────────────────────────────────────────────────────────────────
//보행중 색상 패치 인식 못하면 수정 
#define SEEK_DX_DEFAULT    40     // 아직 못 쟀을 때 쓸 값 (화면의 1/8)
#define SEEK_MIN            3     // 아무리 적어도 이만큼은 돈다
#define SEEK_MAX           20     // 아무리 많아도 이 이상은 안 돈다
#define SEEK_CONFIRM_N      2     // 몇 장 연속 보여야 "찾았다" 로 인정
//──────────────────────────────────────────────────────────────────────

/*  ── 출발 제스처 ──────────────────────────────────────────────────────
 *  심판이 신호와 동시에 시계를 누른다.  그래서 신호 뒤의 시간을 최대한 줄인다.
 *
 *     [신호 전]  가리고 기다린다  →  준비 완료           ← 여기까지 미리 끝냄
 *     [신 호 ]  손을 뗀다        →  0.1초 뒤 출발       ← 신호 후 지연 0.1초
 *  ────────────────────────────────────────────────────────────────────── */

#define GEST_SEE_MS       500UL   // 이만큼 보여야 "패치 확인"
#define GEST_COVER_MS     500UL   // 이만큼 안 보여야 "준비 완료"
#define START_CONFIRM_N     2     // 손 뗀 뒤 몇 장 보이면 출발할지


/*  ── 그 밖의 내부값 ──────────────────────────────────────────────────  */
#define AVG_N               5     // 몇 개를 평균낼지 (흔들림 제거)
#define MERGE_GAP          12     // 쪼개진 조각을 합칠 간격 (픽셀)
#define MIN_W              12     // 이보다 작으면 노이즈로 무시(픽셀) 
#define LOST_N              5     // 몇 프레임 연속 안 보이면 "완전 상실"
#define MAX_TURNS           8     // 연속 턴 상한. 넘으면 그냥 걷는다
#define MAX_FINISH_MS  20000UL    // 마무리 직진 최대 시간
#define STOP_REPEAT         5     // 정지키 반복 (단방향이라 유실 대비)
#define STOP_GAP_MS       120
#define HOLD_LOST_MS    5000UL    // 서 있는 중 이만큼 패치가 안 보이면 찾기 시작
#define WAIT_PRINT_MS     500     // 대기 중 화면 갱신 주기
#define RUN_PRINT_MS      250     // 주행 중 화면 갱신 주기

/*  다시 출발할 거리.  패치가 이보다 멀리 보이면 "새 경기" 로 보고 출발한다.
 *  학생이 로봇을 집어 START 에 놓기만 하면 저절로 달린다.  */
#define RESTART_MM     (TRACK_MM / 2)


/* ==========================================================================
 *  값 검사  -  범위를 벗어나면 업로드 자체가 막힌다
 * ========================================================================== */

#if (SIG_NO < 1) || (SIG_NO > 7)
  #error "[1] 색번호(SIG_NO)는 1~7 사이여야 합니다."
#endif
#if (TRACK_MM < 300) || (TRACK_MM > 10000)
  #error "[2] TRACK_MM(출발선~결승선)이 이상합니다. 300~10000 사이여야 합니다."
#endif
#if (FINISH_MM < 200) || (FINISH_MM > 3000)
  #error "[3] FINISH_MM(결승선~패치)이 이상합니다. 200~3000 사이여야 합니다."
#endif
#if (CROSS_MM < 50) || (CROSS_MM > 2000)
  #error "[4] CROSS_MM(결승선을 넘어 더 갈 거리)이 이상합니다. 50~2000 사이여야 합니다."
#endif
#if ((FINISH_MM) - (CROSS_MM)) < 300
  #error "[3][4] FINISH_MM - CROSS_MM 이 300 보다 작습니다. 로봇이 패치에 부딪힙니다."
#endif
#if (PATCH_MM < 50) || (PATCH_MM > 2000)
  #error "[5] PATCH_MM(패치 가로폭)이 이상합니다."
#endif
#if (LENS_K < 20) || (LENS_K > 2000)
  #error "[6] LENS_K(카메라값)가 이상합니다. 1m 앞에서 다시 재세요."
#endif
#if (CAL_DIST_MM < 200) || (CAL_DIST_MM > 3000)
  #error "[6-1] CAL_DIST_MM(교정 거리)이 이상합니다. 200~3000 사이여야 합니다."
#endif
#if (LANE_MM < 30) || (LANE_MM > 700)// 수정함
  #error "[7] LANE_MM 이 이상합니다. 30~500 사이여야 합니다."
#endif
#if (TURN_WAIT_MS < 100) || (TURN_WAIT_MS > 5000)
  #error "[8] TURN_WAIT_MS 가 이상합니다."
#endif
#if (WALK_SPEED < 10) || (WALK_SPEED > 2000)
  #error "[9] WALK_SPEED 가 이상합니다."
#endif
#if (GETUP_MS < 1000) || (GETUP_MS >= FALL_MAX_MS)
  #error "GETUP_MS 는 1000 이상, FALL_MAX_MS 보다 작아야 합니다."
#endif
#if (IMU_UP_DEG >= IMU_FALL_DEG)
  #error "IMU_UP_DEG 는 IMU_FALL_DEG 보다 작아야 합니다."
#endif

/*  카메라 짐작은 더 확실한 방법이 없을 때만 쓴다  */
#if (USE_CAM_FALL == 1) && (USE_IMU_FALL == 0) && (USE_ROBOT_FEEDBACK == 0)
  #define CAM_FALL_ON  1
#else
  #define CAM_FALL_ON  0
#endif


/* ==========================================================================
 *  하드웨어
 * ========================================================================== */

#define PIN_BT_RX   A1            // 블루투스 TX -> 아두이노 (로봇 소식 받기)
#define PIN_BT_TX   A0            // 아두이노 -> 블루투스 RX (명령 보내기)

Pixy2 pixy;
SoftwareSerial D_Serial(PIN_BT_RX, PIN_BT_TX);   // (RX핀, TX핀) 순서

#if (USE_IMU_FALL == 1)
  #include <Wire.h>                 // MPU6050 : SDA=A4  SCL=A5
  #define IMU_ADDR        0x68
#endif


/* ==========================================================================
 *  상태
 * ========================================================================== */

enum { ST_WAIT, ST_RUN, ST_SEEK, ST_FINISH, ST_FALLEN, ST_HOLD };
enum { GEST_SEE, GEST_COVER, GEST_READY };

uint8_t  g_state   = ST_WAIT;
uint8_t  g_gest    = GEST_SEE;
uint32_t g_gestMs  = 0;
uint8_t  g_confirm = 0;           // 출발 직전 연속 확인 장수
int8_t   g_gestShown = -1;
uint8_t  g_lastTip = 255;

/*  카메라값.  아두이노에 저장된 값이 있으면 그것을 쓰고, 없으면 LENS_K 를 쓴다.  */
int16_t  g_lensK     = LENS_K;
bool     g_lensSaved = false;     // 저장된 값을 쓰는 중인가

bool     g_pixyOk    = false;
bool     g_raceStarted = false;   // 한 번이라도 출발했는가
bool     g_walking   = false;     // 전진키를 내보내는 중인가

/*  카메라에서 읽은 것  */
uint8_t  g_sig       = SIG_NO;
int8_t   g_blockErr  = 0;
bool     g_valid     = false;     // 거리와 방향 모두 믿을 수 있다
bool     g_seenNow   = false;     // 이번 장에 패치가 보이긴 했다
int8_t   g_edge      = 0;         // 0=안잘림  +1=오른쪽에 걸침  -1=왼쪽에 걸침
bool     g_clipped   = false;
int16_t  g_bx = 0, g_bw = 0;      // 병합한 패치의 중심 x / 폭
uint8_t  g_mergeCnt  = 0;
uint8_t  g_lostCnt   = 0;
int8_t   g_lastSide  = 1;         // 마지막에 본 쪽 (+1 오른쪽 / -1 왼쪽)

int16_t  g_bufX[AVG_N], g_bufW[AVG_N];
uint8_t  g_idx = 0, g_cnt = 0;
int16_t  g_fx = 0, g_fw = 0;      // 평균낸 값
int16_t  g_centerX = 158;

long     g_dist = 0;              // 패치까지 거리 (mm)
long     g_err  = 0;              // 빗나감 (+면 패치가 오른쪽 = 우턴 필요)
long     g_lastDist = 0;

/*  시간  */
uint32_t g_runStartMs   = 0;
uint32_t g_lastSendMs   = 0;
uint32_t g_lastPrintMs  = 0;
uint32_t g_holdUntil    = 0;      // 이 시각까지는 카메라를 안 본다
uint32_t g_finishUntil  = 0;
bool     g_finishResume = false;
uint32_t g_holdLostMs   = 0;

/*  턴  */
uint8_t  g_turnCnt = 0;
uint8_t  g_edgeCnt = 0;

/*  턴 크기 자동 측정  */
int16_t  g_dxRight = SEEK_DX_DEFAULT;
int16_t  g_dxLeft  = SEEK_DX_DEFAULT;
bool     g_dxKnown = false;
int16_t  g_xBefore = -1;          // 턴 보내기 직전의 x
int8_t   g_measDir = 0;           // 재는 중인 턴 방향 (+1 우 / -1 좌)
uint8_t  g_badTurn = 0;           // 이상한 측정이 연속 몇 번인지

/*  찾기  */
int8_t   g_seekDir   = 1;
uint8_t  g_seekCnt   = 0;
uint8_t  g_seekLimit = SEEK_MIN;
uint8_t  g_seekPhase = 1;
uint8_t  g_seeCnt    = 0;
bool     g_seekFromHold = false;

/*  넘어짐  */
uint32_t g_fallenMs    = 0;
uint32_t g_finishLeft  = 0;
bool     g_finishSaved = false;

/*  넘어짐 감지 (v13)  */
uint32_t g_lastSeenMs    = 0;     // 패치를 마지막으로 본 시각
bool     g_lastSeenMid   = false; // 그때 패치가 화면 가운데 있었나
bool     g_fallSuspect   = false; // 가운데서 갑자기 사라졌다 (넘어짐 의심)
uint32_t g_fallSuspectMs = 0;
bool     g_fallByCam     = false; // 카메라 짐작으로 넘어짐에 들어왔다

#if (USE_IMU_FALL == 1)
bool     g_imuOk     = false;
float    g_refX = 0, g_refY = 0, g_refZ = 1;   // "똑바로 섰을 때" 중력 방향
int16_t  g_tiltDeg   = 0;
uint32_t g_imuReadMs = 0;
uint32_t g_tiltSince = 0;         // 기운 상태가 시작된 시각 (0 = 안 기움)
uint32_t g_upSince   = 0;         // 바로 선 상태가 시작된 시각 (0 = 안 섬)
#endif


/* ==========================================================================
 *  기본 도구
 * ========================================================================== */

long myAbs(long v) { return (v < 0) ? -v : v; }

bool tipChanged(uint8_t id)
{
  if (g_lastTip == id) return false;
  g_lastTip = id;
  return true;
}


/* ==========================================================================
 *  카메라값(LENS_K) 저장 / 불러오기
 *
 *  아두이노 안에는 전원을 꺼도 지워지지 않는 작은 저장 공간이 있다.
 *  거기에 이 로봇만의 카메라값을 넣어 둔다.
 *  → 로봇마다 값이 달라도, 코드는 모든 학생이 똑같은 것을 쓰면 된다.
 *
 *  ★ 이 저장은 프로그램을 새로 업로드해도 지워지지 않는다.
 *     코드의 LENS_K 로 되돌리려면 반드시  x  명령을 써야 한다.
 * ========================================================================== */

#define EE_MAGIC        0xA7      // "여기에 값이 들어 있다" 는 표시
#define EE_ADDR_MAGIC      0
#define EE_ADDR_LENSK      1

#define LENS_MIN          20      // 이 범위를 벗어난 값은 받지 않는다
#define LENS_MAX        2000
#define LENS_STEP          5      // + - 로 한 번에 움직일 크기

void lensLoad()
{
  if (EEPROM.read(EE_ADDR_MAGIC) != EE_MAGIC) { g_lensSaved = false; g_lensK = LENS_K; return; }

  int16_t v = 0;
  EEPROM.get(EE_ADDR_LENSK, v);
  if (v < LENS_MIN || v > LENS_MAX) { g_lensSaved = false; g_lensK = LENS_K; return; }

  g_lensK = v;
  g_lensSaved = true;
}

void lensStore(int16_t v)
{
  g_lensK = v;
  g_lensSaved = true;
  EEPROM.put(EE_ADDR_LENSK, v);
  EEPROM.update(EE_ADDR_MAGIC, EE_MAGIC);
}

void lensClear()
{
  EEPROM.update(EE_ADDR_MAGIC, 0xFF);
  g_lensK = LENS_K;
  g_lensSaved = false;
}

/*  지금 화면을 보고 카메라값을 계산해 저장한다.
 *  "패치가 지금 CAL_DIST_MM 앞에 있다" 고 믿고 역산하는 것이다.  */
void lensCalibrate()
{
  Serial.println();

  if (!g_seenNow) {
    Serial.println(F("[교정 실패] 패치가 안 보입니다."));
    Serial.println(F("   패치가 잘 보이게 놓고 다시 k 를 누르세요."));
    return;
  }
  if (g_edge != 0) {
    Serial.println(F("[교정 실패] 패치가 화면 끝에 걸쳐 있습니다."));
    Serial.println(F("   로봇을 돌려 패치를 화면 가운데로 오게 하세요."));
    return;
  }
  if (!g_valid || g_fw < 1) {
    Serial.println(F("[교정 실패] 아직 측정 중입니다. 1초 뒤에 다시 누르세요."));
    return;
  }

  long v = ((long)g_fw * (long)CAL_DIST_MM) / (long)PATCH_MM;

  if (v < LENS_MIN || v > LENS_MAX) {
    Serial.print(F("[교정 실패] 계산된 값이 이상합니다 ("));  Serial.print(v);
    Serial.println(F(")"));
    Serial.println(F("   패치 크기(PATCH_MM)와 놓은 거리(CAL_DIST_MM)를 확인하세요."));
    return;
  }

  lensStore((int16_t)v);

  Serial.print(F("[교정 완료] 폭 "));      Serial.print(g_fw);
  Serial.print(F("px  x  거리 "));         Serial.print((long)CAL_DIST_MM);
  Serial.print(F("mm  /  패치 "));         Serial.print((long)PATCH_MM);
  Serial.print(F("mm   ->   LENS_K = "));  Serial.println(g_lensK);
  Serial.println(F("   아두이노에 저장했습니다. 전원을 꺼도 남습니다."));
}

/*  + - 로 손보기.  누를 때마다 바로 저장한다.  */
void lensNudge(int16_t delta)
{
  long v = (long)g_lensK + delta;
  if (v < LENS_MIN) v = LENS_MIN;
  if (v > LENS_MAX) v = LENS_MAX;

  lensStore((int16_t)v);

  Serial.print(F("\n[조정] LENS_K = "));  Serial.print(g_lensK);
  if (g_valid) { Serial.print(F("   지금 거리 "));  Serial.print(g_dist);  Serial.print(F("mm")); }
  Serial.println();
}

void lensPrint()
{
  Serial.print(F("LENS_K      : "));  Serial.print(g_lensK);
  if (g_lensSaved) Serial.println(F("  (저장된 값)"));
  else             Serial.println(F("  (코드 값 - 아직 교정 안 함)"));
}


/* ==========================================================================
 *  로봇에게 모션 키를 보낸다
 * ========================================================================== */

void sendMotion(unsigned long code, bool showHex)
{
  unsigned char buf[24];
  unsigned char chksum = 0;
  int len = 0, i;

  buf[len++] = 0xAA;                                  // SOH
  buf[len++] = (unsigned char)ROBOT_ID;               // 국번
  buf[len++] = 0xD1;                                  // main function
  buf[len++] = 0;                                     // sub function 1
  buf[len++] = 0;                                     // sub function 2
  buf[len++] = 0;                                     // data length (H)
  buf[len++] = 4 + 2;                                 // data length (L)

  buf[len++] = (unsigned char)((code >> 24) & 0xff);
  buf[len++] = (unsigned char)((code >> 16) & 0xff);
  buf[len++] = (unsigned char)((code >>  8) & 0xff);
  buf[len++] = (unsigned char)((code >>  0) & 0xff);

  buf[len++] = (unsigned char)((SPEED_OVR >> 8) & 0xff);
  buf[len++] = (unsigned char)((SPEED_OVR >> 0) & 0xff);

  // ★[확인] 체크섬 계산식. v5 에서 가져온 것이라 규격서 대조가 필요하다.
  for (i = 1; i < len; i++) chksum = (chksum ^ buf[i]) + 1;
  buf[len++] = chksum;

  for (i = 0; i < len; i++) D_Serial.write(buf[i]);

  if (showHex) {
    Serial.print(F("   [패킷] "));
    for (i = 0; i < len; i++) {
      if (buf[i] < 0x10) Serial.print('0');
      Serial.print(buf[i], HEX);  Serial.print(' ');
    }
    Serial.println();
  }
}


/* ==========================================================================
 *  평균 (흔들림 제거)
 * ========================================================================== */

void avgReset() { g_idx = 0; g_cnt = 0; }
bool avgReady() { return (g_cnt >= AVG_N); }

void avgPush(int16_t x, int16_t w)
{
  g_bufX[g_idx] = x;
  g_bufW[g_idx] = w;
  g_idx = (uint8_t)((g_idx + 1) % AVG_N);
  if (g_cnt < AVG_N) g_cnt++;
}


/* ==========================================================================
 *  패치 찾아 조각 합치기
 *
 *  Pixy2 는 패치 하나를 2~3 조각으로 쪼개서 보고한다. 반드시 합쳐야 한다.
 * ========================================================================== */

bool acquirePatch()
{
  g_mergeCnt = 0;
  g_edge = 0;

  int16_t a = -1;
  int16_t bestW = 0;
  for (uint8_t i = 0; i < pixy.ccc.numBlocks; i++) {
    if (pixy.ccc.blocks[i].m_signature != g_sig) continue;
    if (pixy.ccc.blocks[i].m_width < MIN_W)      continue;
    if (pixy.ccc.blocks[i].m_width > bestW) { bestW = pixy.ccc.blocks[i].m_width; a = (int16_t)i; }
  }
  if (a < 0) return false;

  int16_t L = pixy.ccc.blocks[a].m_x - pixy.ccc.blocks[a].m_width  / 2;
  int16_t R = pixy.ccc.blocks[a].m_x + pixy.ccc.blocks[a].m_width  / 2;
  int16_t T = pixy.ccc.blocks[a].m_y - pixy.ccc.blocks[a].m_height / 2;
  int16_t B = pixy.ccc.blocks[a].m_y + pixy.ccc.blocks[a].m_height / 2;
  g_mergeCnt = 1;

  // 흡수한 조각에 또 다른 조각이 붙어 있을 수 있어 세 번 훑는다
  for (uint8_t pass = 0; pass < 3; pass++) {
    for (uint8_t i = 0; i < pixy.ccc.numBlocks; i++) {
      if ((int16_t)i == a) continue;
      if (pixy.ccc.blocks[i].m_signature != g_sig) continue;

      int16_t l = pixy.ccc.blocks[i].m_x - pixy.ccc.blocks[i].m_width  / 2;
      int16_t r = pixy.ccc.blocks[i].m_x + pixy.ccc.blocks[i].m_width  / 2;
      int16_t t = pixy.ccc.blocks[i].m_y - pixy.ccc.blocks[i].m_height / 2;
      int16_t b = pixy.ccc.blocks[i].m_y + pixy.ccc.blocks[i].m_height / 2;

      if (r < L - MERGE_GAP || l > R + MERGE_GAP) continue;
      if (b < T - MERGE_GAP || t > B + MERGE_GAP) continue;

      if (l < L) L = l;
      if (r > R) R = r;
      if (t < T) T = t;
      if (b > B) B = b;
      if (pass == 0) g_mergeCnt++;
    }
  }

  g_bw = R - L;
  g_bx = (L + R) / 2;

  /*  화면 좌우로 잘렸는가.
   *
   *  잘리면 폭이 작게 읽혀 "아직 멀다" 고 오판하므로 거리는 못 쓴다.
   *  하지만 "패치가 어느 쪽에 있다" 는 것은 100% 확실하다.
   *  v10 은 이걸 통째로 버렸지만, v11 은 방향으로 살려 쓴다.
   *  ( 완전히 잃기 전에 되돌리는 것이 훨씬 쉽기 때문이다 )  */
  if      (L <= 1)                       g_edge = -1;   // 왼쪽에 걸침
  else if (R >= pixy.frameWidth - 2)     g_edge = +1;   // 오른쪽에 걸침
  g_clipped = (g_edge != 0);

  return (g_bw >= MIN_W);
}


/* ==========================================================================
 *  턴 한 번이 화면에서 몇 픽셀인지 스스로 잰다
 *
 *  로봇이 오른쪽으로 돌면 화면 속 물체는 왼쪽으로 흐른다.  (x 가 줄어든다)
 *  로봇이 왼쪽으로  돌면 화면 속 물체는 오른쪽으로 흐른다. (x 가 늘어난다)
 *
 *  이 값은 거리와 상관없이 일정하다.  회전은 각도라서 픽셀 이동량이 같다.
 *  그래서 도(°)를 몰라도 되고, 로봇마다 재줄 필요도 없다.
 * ========================================================================== */

void measureTurn()
{
  int16_t d   = g_fx - g_xBefore;
  int16_t mag = (d < 0) ? -d : d;
  bool    ok  = (g_measDir > 0) ? (d < 0) : (d > 0);

  if (mag < 5) {
    /*  거의 안 움직였다  */
    if (++g_badTurn == 2) {
      Serial.println(F("\n[!] 턴을 보냈는데 로봇이 거의 안 돕니다."));
      Serial.println(F("    - 블루투스가 연결됐는지"));
      Serial.println(F("    - KEY_LEFT / KEY_RIGHT 번호가 맞는지 확인하세요."));
      g_badTurn = 0;
    }
    return;
  }

  if (!ok) {
    /*  반대로 돌았다  */
    if (++g_badTurn == 2) {
      Serial.println(F("\n[!] 좌우가 반대입니다."));
      Serial.println(F("    KEY_LEFT 와 KEY_RIGHT 번호를 서로 바꿔 넣으세요."));
      g_badTurn = 0;
    }
    return;
  }

  g_badTurn = 0;
  g_dxKnown = true;

  /*  이동평균. 갑작스런 값에 휘둘리지 않게 천천히 따라간다.  */
  if (g_measDir > 0) g_dxRight = (int16_t)(((long)g_dxRight * 2 + mag) / 3);
  else               g_dxLeft  = (int16_t)(((long)g_dxLeft  * 2 + mag) / 3);
}


/* ==========================================================================
 *  카메라 읽기
 * ========================================================================== */

/*  이번 장에 패치가 안 보였다.
 *  방금 전까지 화면 가운데 있었는데 한순간에 사라졌다면 넘어짐을 의심한다.
 *  ( 턴 대기 중에는 카메라를 안 읽으므로 g_lastSeenMs 가 오래돼서 여기 안 걸린다 )  */
void noteLost()
{
  if (g_lostCnt < 255) g_lostCnt++;
  g_edge = 0;

#if (CAM_FALL_ON == 1)
  if (g_lostCnt == 1 && !g_fallSuspect && g_lastSeenMid
      && (millis() - g_lastSeenMs) <= FALL_GONE_MS) {
    g_fallSuspect   = true;
    g_fallSuspectMs = millis();
  }
#endif
}

void updateVision()
{
  g_blockErr = pixy.ccc.getBlocks();
  g_valid = false;  g_seenNow = false;

  if (g_blockErr < 0)  { noteLost(); return; }
  if (!acquirePatch()) { noteLost(); return; }

  /*  여기까지 왔으면 패치가 보이긴 한다  */
  g_seenNow = true;
  g_lostCnt = 0;
  g_lastSide = (g_bx > g_centerX) ? +1 : -1;

  g_lastSeenMs  = millis();
  g_lastSeenMid = (g_edge == 0) && (myAbs((long)g_bx - g_centerX) <= FALL_CENTER_PX);

  /*  화면 끝에 걸쳤으면 거리는 못 쓴다.  방향만 남긴다.  */
  if (g_edge != 0) { avgReset(); return; }

  avgPush(g_bx, g_bw);
  if (!avgReady()) return;

  long sx = 0, sw = 0;
  for (uint8_t i = 0; i < AVG_N; i++) { sx += g_bufX[i]; sw += g_bufW[i]; }
  g_fx = (int16_t)(sx / AVG_N);
  g_fw = (int16_t)(sw / AVG_N);
  if (g_fw < 1) return;

  /*  거리 = 패치폭 x 카메라값 / 화면에서 본 폭  */
  g_dist = ((long)PATCH_MM * (long)g_lensK) / (long)g_fw;
  g_err  = ((long)(g_fx - g_centerX) * (long)PATCH_MM) / (long)g_fw;
  g_lastDist = g_dist;
  g_valid = true;
  g_lastSide = (g_fx > g_centerX) ? +1 : -1;

  /*  턴을 보내놓고 결과를 기다리던 중이면 여기서 잰다  */
  if (g_measDir != 0 && g_xBefore >= 0) {
    measureTurn();
    g_measDir = 0;
    g_xBefore = -1;
  }
}


/* ==========================================================================
 *  상태 전환
 * ========================================================================== */

const __FlashStringHelper* stateName()
{
  switch (g_state) {
    case ST_WAIT:   return F("대기");
    case ST_RUN:    return F("주행");
    case ST_SEEK:   return F("찾기");
    case ST_FINISH: return F("마무리");
    case ST_FALLEN: return F("넘어짐");
    default:        return F("서있음");
  }
}

void gotoState(uint8_t s)
{
  g_state     = s;
  g_turnCnt   = 0;
  g_edgeCnt   = 0;
  g_holdUntil = 0;
  g_lastTip   = 255;
  g_measDir   = 0;
  g_xBefore   = -1;
  g_fallSuspect = false;
  avgReset();

  /*  주행에 들어가면 일단 안 걷는 상태로 시작한다.
   *  그러면 빗나감이 큰 경우 저절로 제자리 정렬부터 하게 된다.  */
  g_walking = (s == ST_FINISH);

  Serial.print(F("\n===== "));  Serial.print(stateName());  Serial.println(F(" ====="));
}


/* ==========================================================================
 *  서 있기
 *
 *  v10 의 "정지" 를 대신한다.  다른 점은 죽지 않는다는 것이다.
 *  패치가 멀리 보이면 저절로 다시 출발한다.
 *  → 학생이 로봇을 집어 START 에 놓기만 하면 된다. 버튼도 리셋도 없다.
 * ========================================================================== */

void enterHold(const __FlashStringHelper* why)
{
  Serial.print(F("\n>> 섭니다: "));  Serial.println(why);
#if (USE_STOP_KEY == 1)
  for (uint8_t i = 0; i < STOP_REPEAT; i++) { sendMotion(KEY_STOP, false); delay(STOP_GAP_MS); }
#endif
  gotoState(ST_HOLD);
  g_holdLostMs = millis();
  Serial.print(F("   로봇을 START 라인에 놓으면 저절로 다시 출발합니다. (패치가 "));
  Serial.print((long)RESTART_MM);  Serial.println(F("mm 보다 멀리 보이면)"));
}


/* ==========================================================================
 *  마무리 (눈감고 직진) 시작
 * ========================================================================== */

void startFinish(long mm, bool resume)
{
  if (mm < 0) mm = 0;
  uint32_t ms = (uint32_t)((mm * 1000L) / (long)WALK_SPEED);
  if (ms > MAX_FINISH_MS) ms = MAX_FINISH_MS;

  Serial.print(F("   남은 "));    Serial.print(mm);
  Serial.print(F("mm  ->  "));    Serial.print(ms);  Serial.println(F("ms 직진"));

  gotoState(ST_FINISH);
  g_finishResume = resume;
  g_finishUntil  = millis() + ms;
}


/* ==========================================================================
 *  패치 찾기 시작
 *
 *  패치는 그냥 사라지지 않는다. 반드시 화면 한쪽 끝으로 빠져나간다.
 *  마지막에 본 쪽을 기억해 두었다가 그쪽으로 돌면 된다.
 *
 *  몇 번 돌지는 스스로 잰 턴 크기로 계산한다.
 *      필요 턴  =  (화면 폭의 3/4)  /  턴 한 번 픽셀
 *  턴이 약한 로봇은 알아서 많이, 큰 로봇은 알아서 적게 돈다.
 * ========================================================================== */

uint8_t seekLimit(int8_t dir)
{
  int16_t dx = (dir > 0) ? g_dxRight : g_dxLeft;
  if (dx < 1) dx = 1;

  long n = ((long)pixy.frameWidth * 3L / 4L) / (long)dx;
  if (n < SEEK_MIN) n = SEEK_MIN;
  if (n > SEEK_MAX) n = SEEK_MAX;
  return (uint8_t)n;
}

void startSeek(bool fromHold)
{
  gotoState(ST_SEEK);
  g_walking       = false;
  g_seekFromHold  = fromHold;
  g_seekDir       = (g_lastSide >= 0) ? +1 : -1;
  g_seekPhase     = 1;
  g_seekCnt       = 0;
  g_seeCnt        = 0;
  g_seekLimit     = seekLimit(g_seekDir);

  Serial.print(F("   패치를 놓쳤습니다. 마지막에 "));
  Serial.print((g_seekDir > 0) ? F("오른쪽") : F("왼쪽"));
  Serial.println(F(" 에 있었습니다."));
  Serial.print(F("   그쪽으로 최대 "));  Serial.print(g_seekLimit);
  Serial.print(F("번 돌며 찾습니다.  (턴 1회 = 약 "));
  Serial.print((g_seekDir > 0) ? g_dxRight : g_dxLeft);
  Serial.print(F("픽셀"));
  Serial.println(g_dxKnown ? F(", 실측)") : F(", 아직 안 재봄)"));
}


/* ==========================================================================
 *  넘어짐  (로봇이 자이로로 알려준 경우)
 * ========================================================================== */

void enterFallen()
{
  /*  마무리 중이었다면 시계를 멈춰 둔다.
   *  누워 있는 동안 시간이 흐르면 결승선을 안 넘고도 다 간 걸로 친다.  */
  if (g_state == ST_FINISH) {
    g_finishLeft  = (g_finishUntil > millis()) ? (g_finishUntil - millis()) : 0;
    g_finishSaved = true;
  }
  gotoState(ST_FALLEN);
  g_walking  = false;
  g_fallenMs = millis();
  Serial.println(F("   로봇이 넘어졌습니다. 일어날 때까지 아무 키도 안 보냅니다."));
  if (g_fallByCam) {
    Serial.print(F("   (카메라 짐작 : 가운데 있던 패치가 갑자기 사라짐)  "));
    Serial.print(GETUP_MS / 1000);  Serial.println(F("초 기다립니다."));
  }
  if (g_finishSaved) {
    Serial.print(F("   마무리 시계를 멈췄습니다. 남은 "));
    Serial.print(g_finishLeft);  Serial.println(F("ms"));
  }
}

void recoverFallen()
{
  Serial.println(F("\n   로봇이 일어났습니다."));
  g_fallByCam = false;
  if (g_finishSaved) {
    /*  마무리 중이었으면 멈춰둔 시계를 그대로 이어서 간다  */
    gotoState(ST_FINISH);
    g_finishUntil = millis() + g_finishLeft;
    g_finishSaved = false;
    Serial.print(F("   마무리를 이어갑니다. 남은 "));
    Serial.print(g_finishLeft);  Serial.println(F("ms"));
  } else if (g_seenNow) {
    /*  일어나 보니 패치가 보인다. 주행으로 돌아가면 방향부터 맞추고 걷는다.  */
    Serial.println(F("   패치가 보입니다 - 주행 복귀"));
    gotoState(ST_RUN);
  } else {
    /*  일어난 뒤 방향은 완전히 틀어져 있다. 반드시 다시 찾아야 한다.
     *  ★ 이때는 찾기에 실패해도 "눈감고 직진" 하지 않고 서서 다시 찾는다.
     *    방향을 모르는 채로 걸으면 트랙 밖으로 나가기 때문이다.  */
    startSeek(true);
  }
}


/* ==========================================================================
 *  로봇이 보내는 소식 받기
 * ========================================================================== */

void serviceRobot()
{
#if (USE_ROBOT_FEEDBACK == 1)
  static bool mark = false;

  while (D_Serial.available()) {
    char c = (char)D_Serial.read();

    if (c == RX_MARK) { mark = true; continue; }
    if (!mark) continue;              // '#' 없이 온 글자는 잡음으로 본다
    mark = false;

    if (c == RX_FALLEN || c == RX_GETUP) {
      if (g_state != ST_FALLEN && g_state != ST_WAIT) enterFallen();
    }
    else if (c == RX_OK) {
      if (g_state == ST_FALLEN) recoverFallen();
    }
  }
#endif
}


/* ==========================================================================
 *  넘어짐 감지 (v13)  -  로봇이 알려주지 않아도 아두이노가 스스로 안다
 * ========================================================================== */

#if (USE_IMU_FALL == 1)

bool imuRaw(float &x, float &y, float &z)
{
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(0x3B);                                   // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)6) != 6) return false;

  int16_t v[3];
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    v[i] = (int16_t)((hi << 8) | lo);
  }
  x = v[0];  y = v[1];  z = v[2];
  return true;
}

void imuInit()
{
  Wire.begin();
  Wire.setClock(100000);
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(0x6B);  Wire.write(0);                   // 잠 깨우기
  g_imuOk = (Wire.endTransmission() == 0);
}

/*  지금 자세를 "똑바로 선 자세" 로 기억한다  */
void imuSetRef()
{
  if (!g_imuOk) return;
  float sx = 0, sy = 0, sz = 0, x, y, z;
  uint8_t n = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (imuRaw(x, y, z)) { sx += x; sy += y; sz += z; n++; }
    delay(2);
  }
  if (n == 0) { g_imuOk = false; return; }
  float m = sqrt(sx * sx + sy * sy + sz * sz);
  if (m < 1) return;
  g_refX = sx / m;  g_refY = sy / m;  g_refZ = sz / m;
}

/*  "똑바로" 에서 몇 도 기울었는지  */
void imuUpdate()
{
  if (!g_imuOk) return;
  if (millis() - g_imuReadMs < 20) return;
  g_imuReadMs = millis();

  float x, y, z;
  if (!imuRaw(x, y, z)) return;
  float m = sqrt(x * x + y * y + z * z);
  if (m < 1) return;

  float c = (x * g_refX + y * g_refY + z * g_refZ) / m;
  if (c >  1) c =  1;
  if (c < -1) c = -1;
  g_tiltDeg = (int16_t)(acos(c) * 57.2958);
}

#endif

void serviceFall()
{
#if (USE_IMU_FALL == 1)
  imuUpdate();
  if (g_imuOk && g_state != ST_WAIT) {
    uint32_t now = millis();

    if (g_tiltDeg >= IMU_FALL_DEG) { if (g_tiltSince == 0) g_tiltSince = now; }
    else                             g_tiltSince = 0;
    if (g_tiltDeg <= IMU_UP_DEG)   { if (g_upSince   == 0) g_upSince   = now; }
    else                             g_upSince   = 0;

    if (g_state != ST_FALLEN) {
      if (g_tiltSince && now - g_tiltSince >= IMU_FALL_MS) {
        Serial.print(F("\n   [MPU6050] 기울기 "));  Serial.print(g_tiltDeg);  Serial.println(F("도"));
        enterFallen();
      }
    } else {
      if (g_upSince && now - g_upSince >= IMU_UP_MS) recoverFallen();
    }
  }
#endif

#if (CAM_FALL_ON == 1)
  /*  주행 중 패치가 가운데서 갑자기 사라졌다  */
  if (g_state == ST_RUN && g_fallSuspect) {
    if (g_seenNow) {
      g_fallSuspect = false;                              // 잠깐 깜빡인 것
    } else if (millis() - g_fallSuspectMs >= FALL_CONFIRM_MS) {
      g_fallByCam = true;
      enterFallen();
    }
  }

  /*  일어날 시간을 다 기다렸다  */
  if (g_state == ST_FALLEN && g_fallByCam && millis() - g_fallenMs >= GETUP_MS) {
    recoverFallen();
  }
#endif
}


/* ==========================================================================
 *  ① 대기  -  첫 출발 전에만 들른다
 *
 *  대기 중에는 로봇이 어차피 안 움직이므로, 여기서 카메라 값을 그대로
 *  보여준다.  LENS_K 를 잴 때도 이 화면을 쓴다.
 * ========================================================================== */

void gestReset() { g_gest = GEST_SEE; g_gestMs = millis(); g_gestShown = -1; g_confirm = 0; g_lastTip = 255; }

void doStart()
{
  Serial.println(F("\n*** 출발 ***"));
  g_raceStarted = true;
  g_runStartMs  = millis();
  gotoState(ST_RUN);
#if (USE_IMU_FALL == 1)
  imuSetRef();                      // 출발선에 선 자세 = 똑바로
#endif
}

void printWait()
{
  static uint32_t last = 0;
  if (millis() - last < WAIT_PRINT_MS) return;
  last = millis();

  Serial.print(F("[대기] "));

  if (!g_seenNow) {
    Serial.println(F("패치 안 보임"));
    if (tipChanged(1)) {
      Serial.println(F("   해결: 1) 패치가 카메라 앞에 있는지"));
      Serial.println(F("         2) PixyMon 에서 색을 학습했는지"));
      Serial.print  (F("         3) 색번호가 "));  Serial.print(g_sig);
      Serial.println(F(" 번이 맞는지"));
    }
  } else if (g_edge != 0) {
    Serial.print(F("패치가 화면 "));
    Serial.print((g_edge > 0) ? F("오른쪽") : F("왼쪽"));
    Serial.println(F(" 끝에 걸쳐 있습니다. 로봇을 돌려 가운데로 맞추세요."));
  } else if (!g_valid) {
    Serial.println(F("측정 중..."));
  } else {
    Serial.print(F("폭 "));      Serial.print(g_fw);
    Serial.print(F("px  거리 "));Serial.print(g_dist);
    Serial.print(F("mm  빗나감 "));Serial.print(g_err);
    Serial.print(F("mm  조각 ")); Serial.print(g_mergeCnt);
    Serial.println();
  }

  /*  제스처 단계 안내  */
  if (g_gestShown != (int8_t)g_gest) {
    g_gestShown = (int8_t)g_gest;
    switch (g_gest) {
      case GEST_SEE:
        Serial.println(F(" >> 패치가 잘 보이게 로봇을 놓으세요."));
        break;
      case GEST_COVER:
        Serial.println(F(" >> 이제 카메라 앞을 손으로 가리고 계세요."));
        break;
      case GEST_READY:
        Serial.println(F("\n *** 준 비 완 료 ***"));
        Serial.println(F(" >> 심판이 시작하면 손을 떼세요. 바로 출발합니다."));
        break;
    }
  }
}

void runWait()
{
  switch (g_gest) {

    case GEST_SEE:                      // 패치가 있는지 먼저 확인 (신호 전)
      if (g_seenNow) {
        if (millis() - g_gestMs >= GEST_SEE_MS) { g_gest = GEST_COVER; g_gestMs = millis(); }
      } else g_gestMs = millis();
      break;

    case GEST_COVER:                    // 가려짐이 안정적으로 유지되면 준비 완료 (신호 전)
      if (!g_seenNow) {
        if (millis() - g_gestMs >= GEST_COVER_MS) { g_gest = GEST_READY; g_confirm = 0; }
      } else g_gestMs = millis();
      break;

    case GEST_READY:                    // 손을 떼는 순간 출발 (신호 후, 약 0.1초)
      if (g_seenNow) {
        if (++g_confirm >= START_CONFIRM_N) doStart();
      } else g_confirm = 0;
      break;
  }
}


/* ==========================================================================
 *  방향 고치기
 *
 *  턴을 보내고 TURN_WAIT_MS 동안은 아무것도 재지 않는다.
 *  도는 중에 재면 값이 엉망이기 때문이다.
 *  ※ 여기에 delay() 를 넣으면 안 된다. 프로그램 전체가 같이 멈춘다.
 * ========================================================================== */

void doTurn()
{
  g_walking = false;              // 전진키 전송이 끊긴다 = 로봇이 선다

  g_turnCnt++;
  if (g_turnCnt > MAX_TURNS) {
    Serial.print(F("   턴 "));  Serial.print(MAX_TURNS);
    Serial.println(F("회 초과 - 그냥 진행"));
    g_turnCnt = 0;
    g_walking = true;
    return;
  }

  /*  턴 크기를 재기 위해 지금 위치를 기억해 둔다  */
  g_xBefore = g_fx;

  if (g_err > 0) { Serial.print(F(">> 우턴  (빗나감 +")); sendMotion(KEY_RIGHT, false); g_measDir = +1; }
  else           { Serial.print(F(">> 좌턴  (빗나감 "));  sendMotion(KEY_LEFT,  false); g_measDir = -1; }
  Serial.print(g_err);  Serial.println(F("mm)"));

  g_holdUntil = millis() + TURN_WAIT_MS;
  avgReset();
}

/*  화면 끝에 걸쳤을 때.
 *  거리는 못 믿지만 방향은 확실하므로 그쪽으로 돌려 가운데로 데려온다.
 *  이 단계에서 붙잡으면 완전히 잃는 일이 거의 없다.  */
void edgeTurn()
{
  g_walking = false;

  g_edgeCnt++;
  if (g_edgeCnt > MAX_TURNS) { startSeek(false); return; }

  Serial.print(F(">> 화면 "));
  Serial.print((g_edge > 0) ? F("오른쪽") : F("왼쪽"));
  Serial.println(F(" 끝에 걸침 - 방향만 보고 되돌림"));

  sendMotion((g_edge > 0) ? KEY_RIGHT : KEY_LEFT, false);

  g_holdUntil = millis() + TURN_WAIT_MS;
  avgReset();
}


/* ==========================================================================
 *  ② 주행
 * ========================================================================== */

void runRun()
{
  //  턴 동작이 끝날 때까지는 아무것도 하지 않는다
  if (millis() < g_holdUntil) return;

  //  가운데서 갑자기 사라졌다 -> 넘어졌는지 확인하는 동안은 찾기로 가지 않는다
  if (g_fallSuspect && !g_seenNow) return;

  //  완전히 잃었다 -> 찾기로
  if (g_lostCnt >= LOST_N) { startSeek(false); return; }

  //  화면 끝에 걸쳤다
  if (!g_valid && g_edge != 0) {
    /*  패치가 화면 절반을 넘게 차지하면 잘린 게 아니라 아주 가까운 것이다  */
    if (g_bw >= pixy.frameWidth / 2) {
      Serial.println(F("   패치가 코앞입니다 - 마무리로"));
      startFinish(CROSS_MM, false);
      return;
    }
    edgeTurn();
    return;
  }

  if (!g_valid) return;
  g_edgeCnt = 0;

  //  결승선 도달.
  //  더 가까워지면 패치가 화면을 벗어나 판정이 무너지므로,
  //  가장 믿을 수 있는 시점에 끊고 나머지는 시간으로 간다.
  if (g_dist <= FINISH_MM) {
    Serial.print(F("   결승선 도달 "));  Serial.print(g_dist);  Serial.println(F("mm"));
    startFinish(CROSS_MM, false);
    return;
  }

  /*  방향 판정.
   *  걷는 중이면 LANE_MM 을 넘을 때 보정을 시작하고,
   *  보정 중이면 그 절반 밑으로 와야 다시 걷는다.
   *  기준을 다르게 둬야 경계에서 걷다 멈추다를 반복하지 않는다.  */
  long limit = g_walking ? (long)LANE_MM : ((long)LANE_MM / 2);

  if (myAbs(g_err) > limit) {
    doTurn();
    return;
  }

  if (!g_walking) {
    Serial.print(F("   방향 맞음 (빗나감 "));  Serial.print(g_err);
    Serial.println(F("mm) - 전진"));
    g_walking = true;
  }
  g_turnCnt = 0;
}


/* ==========================================================================
 *  ③ 찾기
 *
 *   1단계 : 마지막에 본 쪽으로 보일 때까지 돈다
 *   2단계 : 그래도 없으면 반대로 넓게 훑는다 (1단계의 2배)
 *   3단계 : 그래도 없으면 포기하고 앞으로 간다
 *           ( 미완주는 "움직인 거리" 로 순위를 매기므로
 *             제자리에서 도는 것보다 앞으로 가는 게 이득이다 )
 * ========================================================================== */

void runSeek()
{
  if (millis() < g_holdUntil) return;

  /*  찾았다  */
  if (g_seenNow) {
    if (++g_seeCnt >= SEEK_CONFIRM_N) {
      Serial.println(F("   패치를 다시 찾았습니다 - 주행 복귀"));
      gotoState(ST_RUN);
    }
    return;
  }
  g_seeCnt = 0;

  /*  이 방향은 다 훑었다  */
  if (g_seekCnt >= g_seekLimit) {

    if (g_seekPhase == 1) {
      g_seekPhase = 2;
      g_seekDir   = -g_seekDir;
      g_seekCnt   = 0;
      g_seekLimit = (uint8_t)(seekLimit(g_seekDir) * 2);
      if (g_seekLimit > SEEK_MAX * 2) g_seekLimit = SEEK_MAX * 2;

      Serial.print(F("   그쪽엔 없습니다. 반대("));
      Serial.print((g_seekDir > 0) ? F("오른쪽") : F("왼쪽"));
      Serial.print(F(")로 최대 "));  Serial.print(g_seekLimit);
      Serial.println(F("번 훑습니다."));
      return;
    }

    /*  2단계도 실패  */
    if (g_seekFromHold) {
      Serial.println(F("   패치를 못 찾았습니다 - 다시 서서 기다립니다"));
      enterHold(F("패치 못 찾음"));
      return;
    }
    Serial.println(F("   패치를 못 찾았습니다 - 멈추지 않고 앞으로 갑니다"));
    long mm = (g_lastDist > (long)FINISH_MM)
              ? (g_lastDist - (long)FINISH_MM + (long)CROSS_MM)
              : (long)CROSS_MM;
    startFinish(mm, true);
    return;
  }

  /*  한 번 더 돈다  */
  sendMotion((g_seekDir > 0) ? KEY_RIGHT : KEY_LEFT, false);
  g_seekCnt++;
  g_holdUntil = millis() + TURN_WAIT_MS;
  avgReset();
}


/* ==========================================================================
 *  ④ 마무리
 * ========================================================================== */

void runFinish()
{
  if (g_finishResume && g_valid && g_dist > FINISH_MM) {
    Serial.println(F("   패치가 다시 보입니다 - 주행 복귀"));
    gotoState(ST_RUN);
    return;
  }
  if (millis() >= g_finishUntil) enterHold(F("주행 완료"));
}


/* ==========================================================================
 *  ⑤ 서있기  -  자동 재출발
 * ========================================================================== */

void runHold()
{
  /*  패치가 멀리 보인다 = 학생이 START 에 다시 놓았다 = 새 경기  */
  if (g_valid && g_dist >= (long)RESTART_MM) {
    Serial.print(F("\n*** 패치가 "));  Serial.print(g_dist);
    Serial.println(F("mm 앞에 보입니다 - 다시 출발 ***"));
    g_runStartMs = millis();
    gotoState(ST_RUN);
    return;
  }

  if (g_seenNow) { g_holdLostMs = millis(); return; }

  /*  한참 안 보이면 방향이 틀어진 것이므로 찾아본다  */
  if (millis() - g_holdLostMs >= HOLD_LOST_MS) startSeek(true);
}


/* ==========================================================================
 *  넘어져 있는 동안
 * ========================================================================== */

void runFallen()
{
  /*  일어났다는 소식이 끝내 안 오면 (학생이 집어간 경우 등)
   *  마냥 굳어 있지 않고 스스로 다시 움직인다.  */
  if (millis() - g_fallenMs > FALL_MAX_MS) {
    Serial.println(F("\n   일어났다는 소식이 없습니다 - 스스로 다시 찾습니다"));
    g_finishSaved = false;
    g_fallByCam   = false;
    startSeek(false);
  }
}


/* ==========================================================================
 *  전진키 반복 전송
 *
 *  걸어도 되는 상태일 때만 내보낸다. 상태가 바뀌면 전송이 끊겨 로봇이 선다.
 *  ※ 여기에 delay() 를 넣으면 안 된다. 프로그램 전체가 같이 멈춘다.
 * ========================================================================== */

void serviceWalk()
{
  if (g_state != ST_RUN && g_state != ST_FINISH) return;
  if (!g_walking) return;
  if (millis() < g_holdUntil) return;
  if (millis() - g_lastSendMs < WALK_KEY_MS) return;

  g_lastSendMs = millis();
  sendMotion(KEY_FORWARD, false);
}


/* ==========================================================================
 *  주행 중 화면
 * ========================================================================== */

void printRun()
{
  if (millis() - g_lastPrintMs < RUN_PRINT_MS) return;
  g_lastPrintMs = millis();

  Serial.print('[');  Serial.print(stateName());  Serial.print(F("] "));
  Serial.print((millis() - g_runStartMs) / 1000);  Serial.print(F("초  "));

  switch (g_state) {

    case ST_FINISH: {
      long left = (long)(g_finishUntil - millis());
      if (left < 0) left = 0;
      Serial.print(F("눈감고 직진  남은 "));  Serial.print(left);  Serial.println(F("ms"));
      break;
    }

    case ST_SEEK:
      Serial.print((g_seekDir > 0) ? F("오른쪽") : F("왼쪽"));
      Serial.print(F("으로 도는 중  "));
      Serial.print(g_seekCnt);  Serial.print('/');  Serial.print(g_seekLimit);
      Serial.print(F("  ("));  Serial.print(g_seekPhase);  Serial.println(F("단계)"));
      break;

    case ST_FALLEN:
      Serial.print(F("넘어짐 - 일어나는 중  "));
      Serial.print((millis() - g_fallenMs) / 1000);  Serial.print(F("초 경과"));
      if (g_fallByCam) { Serial.print('/');  Serial.print(GETUP_MS / 1000);  Serial.print(F("초")); }
#if (USE_IMU_FALL == 1)
      Serial.print(F("  기울기 "));  Serial.print(g_tiltDeg);  Serial.print(F("도"));
#endif
      Serial.println();
      break;

    case ST_HOLD:
      if (g_valid) { Serial.print(F("서 있음  패치 ")); Serial.print(g_dist); Serial.println(F("mm")); }
      else         { Serial.println(F("서 있음  (패치 안 보임)")); }
      break;

    default:
      if (g_valid) {
        Serial.print(F("거리 "));      Serial.print(g_dist);
        Serial.print(F("mm  빗나감 "));Serial.print(g_err);
        Serial.print(F("mm  "));
        Serial.println(g_walking ? F("전진") : F("정렬"));
      } else if (g_edge != 0) {
        Serial.print(F("화면 "));
        Serial.print((g_edge > 0) ? F("오른쪽") : F("왼쪽"));
        Serial.println(F(" 끝에 걸침"));
      } else if (g_fallSuspect) {
        Serial.println(F("패치가 가운데서 갑자기 사라짐 - 넘어졌는지 확인 중"));
      } else {
        Serial.print(F("패치 안 보임 ("));  Serial.print(g_lostCnt);
        Serial.print('/');  Serial.print(LOST_N);  Serial.println(F(")"));
      }
      break;
  }
}


/* ==========================================================================
 *  시리얼 명령 (선생님용)
 * ========================================================================== */

void serviceSerial()
{
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '?') {
      Serial.println(F("\n  [카메라값 교정]"));
      Serial.print  (F("   k = 지금 패치로 재서 저장  (패치를 "));
      Serial.print((long)CAL_DIST_MM);  Serial.println(F("mm 앞에 놓고)"));
      Serial.println(F("   + - = 5씩 손보기 (자동 저장)    x = 저장 지우기"));
      Serial.println(F("  [그 밖에]"));
      Serial.println(F("   s = 바로출발   r = 대기로   h = 서있기"));
      Serial.println(F("   b = 색없이 직진완주   1~7 = 색번호   t = 턴 측정값"));
      Serial.print  (F("   "));  lensPrint();

    } else if (c == 'k' || c == 'K') {
      lensCalibrate();

    } else if (c == '+' || c == '=') {
      lensNudge(+LENS_STEP);

    } else if (c == '-' || c == '_') {
      lensNudge(-LENS_STEP);

    } else if (c == 'x' || c == 'X') {
      lensClear();
      Serial.println(F("\n[저장 지움] 코드에 적힌 값으로 되돌렸습니다."));
      Serial.print  (F("   "));  lensPrint();

    } else if (c == 's' || c == 'S') {
      if (g_state == ST_WAIT) doStart();

    } else if (c == 'r' || c == 'R') {
      Serial.println(F("\n대기로 되돌립니다"));
      g_raceStarted = false;
      gestReset();
      gotoState(ST_WAIT);

    } else if (c == 'h' || c == 'H') {
      enterHold(F("사용자 요청"));

    } else if (c == 'b' || c == 'B') {
      Serial.println(F("\n### 색 없이 직진 출발 ###"));
      g_raceStarted = true;
      g_runStartMs  = millis();
      startFinish((long)TRACK_MM + (long)CROSS_MM, false);

    } else if (c == 't' || c == 'T') {
      Serial.print(F("\n턴 1회 = 좌 "));  Serial.print(g_dxLeft);
      Serial.print(F("px / 우 "));        Serial.print(g_dxRight);
      Serial.println(g_dxKnown ? F("px  (실측값)") : F("px  (아직 안 재봄)"));

    } else if (c >= '1' && c <= '7') {
      g_sig = (uint8_t)(c - '0');
      Serial.print(F("\n색번호 -> "));  Serial.println(g_sig);
      avgReset();  gestReset();
    }
  }
}


/* ==========================================================================
 *  setup / loop
 * ========================================================================== */

void setup()
{
  Serial.begin(115200);
  D_Serial.begin(115200);
  delay(300);

#if (USE_IMU_FALL == 1)
  imuInit();
  imuSetRef();                      // 출발할 때 다시 잡는다. 여기 값은 임시
#endif

  lensLoad();                       // 저장된 카메라값이 있으면 꺼내 쓴다

  Serial.println(F("\n\n=============================================="));
  Serial.println(F("   arduino_pixy_color_v13   대회 버전"));
  Serial.println(F("=============================================="));

  int8_t r = pixy.init();
  if (r < 0) {
    g_pixyOk = false;
    Serial.println(F("\n[!] 카메라를 찾지 못했습니다."));
    Serial.println(F("    1) Pixy2 케이블이 ICSP 커넥터에 꽂혔는지"));
    Serial.println(F("    2) PixyMon > Settings > Interface 가"));
    Serial.println(F("       \"Arduino ICSP SPI\" 인지 확인하세요."));
    return;
  }
  g_pixyOk = true;
  pixy.ccc.getBlocks();               // frameWidth 를 채우기 위해 한 번 읽는다

  g_centerX = (pixy.frameWidth < 16) ? (158 + CAM_OFFSET)
                                     : (pixy.frameWidth / 2 + CAM_OFFSET);

  Serial.print(F("\n색번호      : "));  Serial.println(g_sig);
  lensPrint();
  Serial.print(F("교정 거리   : "));    Serial.print((long)CAL_DIST_MM);
  Serial.println(F("mm  (여기에 패치를 놓고 k)"));
  Serial.print(F("화면        : "));    Serial.print(pixy.frameWidth);
  Serial.print(F(" x "));               Serial.print(pixy.frameHeight);
  Serial.print(F("   중심 "));          Serial.println(g_centerX);
  Serial.print(F("주로        : "));    Serial.print((long)TRACK_MM);   Serial.println(F("mm"));
  Serial.print(F("결승선~패치 : "));    Serial.print((long)FINISH_MM);  Serial.println(F("mm"));
  Serial.print(F("더 갈 거리  : "));    Serial.print((long)CROSS_MM);   Serial.println(F("mm"));
  Serial.print(F("패치 여유   : "));    Serial.print((long)FINISH_MM - (long)CROSS_MM);
  Serial.println(F("mm  (300 이상이면 안전)"));
  Serial.print(F("다시출발    : 패치가 "));  Serial.print((long)RESTART_MM);
  Serial.println(F("mm 보다 멀리 보이면"));
#if (USE_ROBOT_FEEDBACK == 1)
  Serial.println(F("로봇 소식   : 받음 (#F 넘어짐 / #U 일어나는중 / #S 정상)"));
#else
  Serial.println(F("로봇 소식   : 안 받음"));
  Serial.println(F("   ★ 로봇 펌웨어가 일어나는 중 조종키를 무시해야 합니다."));
#endif
#if (USE_IMU_FALL == 1)
  if (g_imuOk) Serial.println(F("넘어짐 감지 : MPU6050 기울기"));
  else         Serial.println(F("넘어짐 감지 : [!] MPU6050 을 못 찾음 (SDA=A4, SCL=A5 확인)"));
#elif (CAM_FALL_ON == 1)
  Serial.print(F("넘어짐 감지 : 카메라 짐작  (일어나는 시간 "));
  Serial.print(GETUP_MS / 1000);  Serial.println(F("초)"));
#endif
  if (!g_lensSaved) {
    Serial.println(F("\n   ★ 카메라값을 아직 안 쟀습니다."));
    Serial.print  (F("     패치를 "));  Serial.print((long)CAL_DIST_MM);
    Serial.println(F("mm 앞(결승선)에 놓고  k  를 누르세요."));
  }
  Serial.println(F("\n시간 제한 없음 / 멈춤 없음. 도움말은 ? 입니다.\n"));

  gestReset();
  gotoState(ST_WAIT);
}


void loop()
{
  serviceSerial();
  serviceRobot();

  //  카메라를 못 찾은 경우 - 여기서 더 진행하지 않는다
  if (!g_pixyOk) return;

  /*  턴 대기 중에는 카메라를 읽지 않는다.
   *  도는 중의 값이 평균에 섞이면 안 되기 때문이다.  */
  if (millis() >= g_holdUntil) updateVision();

  serviceFall();

  switch (g_state) {
    case ST_WAIT:   runWait();    break;
    case ST_RUN:    runRun();     break;
    case ST_SEEK:   runSeek();    break;
    case ST_FINISH: runFinish();  break;
    case ST_FALLEN: runFallen();  break;
    case ST_HOLD:   runHold();    break;
  }

  serviceWalk();

  if (g_state == ST_WAIT) printWait();
  else                    printRun();
}
