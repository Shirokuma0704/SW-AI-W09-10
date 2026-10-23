<div align="center">

# SW-AI-W09-10 · PintOS Project 3: Virtual Memory

크래프톤 정글 **Week 9–10** 팀 레포예요 (๑˃ᴗ˂)ﻭ
KAIST PintOS의 **Project 3: Virtual Memory**를 2주 동안 같이 구현해요.

![C](https://img.shields.io/badge/C-A8B9CC?style=flat-square&logo=c&logoColor=white)
![x86-64](https://img.shields.io/badge/x86--64-QEMU-FF6600?style=flat-square&logo=qemu&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=flat-square&logo=docker&logoColor=white)
![KAIST PintOS](https://img.shields.io/badge/KAIST-PintOS-004191?style=flat-square)
![Krafton Jungle](https://img.shields.io/badge/Krafton%20Jungle-Week%209--10-111111?style=flat-square)

</div>

## 팀원

<table>
  <tr>
    <td align="center"><a href="https://github.com/Shirokuma0704"><img src="https://github.com/Shirokuma0704.png" width="80" alt="Shirokuma0704"/><br/><sub><b>Shirokuma0704</b></sub></a><br/><sub>메인 레포 관리</sub></td>
    <td align="center"><a href="https://github.com/molru-1"><img src="https://github.com/molru-1.png" width="80" alt="molru-1"/><br/><sub><b>molru-1</b></sub></a><br/><sub>&nbsp;</sub></td>
    <td align="center"><a href="https://github.com/kuk-hwan"><img src="https://github.com/kuk-hwan.png" width="80" alt="kuk-hwan"/><br/><sub><b>kuk-hwan</b></sub></a><br/><sub>&nbsp;</sub></td>
  </tr>
</table>

레포를 포크한 계정 기준으로 적었어요. 역할은 PR로 채워 주세요.

## 한눈에 보기

| | |
|---|---|
| 기간 | 2026-10-23(금) ~ 11-05(목), 2주 |
| 목표 | Project 3 테스트 통과 |
| 구현할 것 | Lazy loading · Page fault 처리 · Supplemental Page Table · Swap in/out |
| 키워드 | Virtual Memory · Page Table · Swap in/out · Page Replacement |
| 주로 고칠 곳 | `pintos/vm/`, `pintos/userprog/` |

구현할 것과 키워드는 정글 과제 안내에 적힌 그대로 옮겼어요.

## 진행 현황

`pintos/vm/build`에서 `make check`를 돌린 결과로 채워요 (ง •̀_•́)ง
배점은 `tests/vm/Grading`, 테스트 수는 각 `Rubric` 파일을 직접 세서 적었어요.

| 묶음 | 테스트 | 배점 | 통과 |
|---|---:|---:|---:|
| VM 기능 (`tests/vm/Rubric.functionality`) | 27 | 60% | – |
| VM 견고성 (`tests/vm/Rubric.robustness`) | 19 | 20% | – |
| User Programs 기능 | 33 | 8% | – |
| User Programs 견고성 | 30 | 5% | – |
| File System base | 13 | 5% | – |
| Threads alarm · priority | 18 | 2% | – |
| **합계** | **140** | **100%** | – |
| Extra: Copy-on-Write (`cow-simple`) | 1 | +25% | – |

VM 테스트 46개를 이름 앞부분으로 나누면 이렇게 돼요.

```
mmap-*   25개
pt-*      8개
page-*    7개
swap-*    4개
lazy-*    2개
```

테스트 하나마다 이슈가 하나씩 있어요. 보드에서 테스트 이름으로 찾으면 돼요.

## 일정

| 날짜 | 일정 |
|---|---|
| 10-23(금) 13:00 | 핀토스 Project 3 기초 교육 |
| 10-23 ~ 10-29 | 1주차: 집중 주간 |
| 11-02(월) 13:30 | OS 강의 |
| 11-03(화) 13:00 | 퀴즈 (PintOS + C, 예정) |
| 11-05(목) 13:00 | 발표 (팀당 한 덱, 한 사람당 2분, 예정) |

"예정"이 붙은 칸은 아직 공지로 확정되지 않았어요.

## 작업 원칙

- 구현은 **AI 에이전트나 코드 생성에 기대지 않고 직접** 해요. 과제 안내에 있는 규칙이에요.
- 1주차(10-23 ~ 10-29)는 반 전체가 **개념 공부까지 AI 없이** 해요.
- 막히면 KAIST 원문, 강의, 팀원에게 먼저 물어봐요.

## 협업 규칙

아직 팀 합의 전이에요 (・ω・)
[#5 팀 협업 룰/팁 정리 및 합의](https://github.com/Shirokuma0704/SW-AI-W09-10/issues/5)에서 정해지면 여기에 채워요.

## 링크

- 보드: [krafton_jungle_week9-10_project](https://github.com/users/Shirokuma0704/projects/6)
- 팀 위키(문서 한국어 요약): [SW-AI-W09-10 Wiki](https://github.com/Shirokuma0704/SW-AI-W09-10/wiki)
- KAIST 원문: https://casys-kaist.github.io/pintos-kaist/

위키는 원문을 요약한 거예요. 중요한 건 원문에서 다시 확인해 주세요.

## 빌드와 테스트

먼저 `pintos` 폴더에서 `activate`를 한 번 실행해요. `pintos/utils`를 PATH에 넣어서 `pintos` 명령을 쓸 수 있게 해 줘요.

```bash
cd pintos
source ./activate
```

Project 3은 `vm` 폴더에서 빌드하고 테스트해요.

```bash
cd pintos/vm
make
cd build
make check                              # 전체 테스트
make tests/vm/lazy-anon.result          # 테스트 하나
```

- 테스트 하나를 돌리면 출력은 `.output`에, 판정은 `.result`에 저장돼요.
- "up to date"라며 다시 안 돌면 그 테스트의 `.output` 파일을 지우고 다시 돌려요.
- 디버깅은 `gdb`로 해요. 방법은 KAIST 원문의 Appendix: Debugging Tools에 있어요.

## 개발 환경 (Docker)

정글에서 준 Docker 환경(`ubuntu:22.04`, x86-64, qemu)을 그대로 써요 ( ˘ω˘ )

1. [Docker Desktop](https://www.docker.com/products/docker-desktop)을 설치하고 실행해요.
2. 이 레포를 포크한 다음, 내 포크를 클론해요.
3. 클론한 폴더를 Dev Container로 열어요.
   - VSCode: `Ctrl+Shift+P` → `Dev Containers: Reopen in Container`
   - CLion: Dev Container로 열기
   - 처음 열 때는 이미지를 빌드하느라 시간이 좀 걸려요.
4. 터미널에서 `pintos` 명령을 못 찾으면, 위 "빌드와 테스트"처럼 `source ./activate`를 직접 실행해요.
   - `.devcontainer/Dockerfile`은 `/workspaces/pintos_22.04_lab_docker/pintos/activate`를 `.bashrc`에서 자동 실행해요. 우리 레포는 폴더 이름이 달라서 이 경로가 없을 수 있어요.

VSCode의 F5 디버깅은 지원하지 않아요. 디버깅은 `gdb`로 해요.
