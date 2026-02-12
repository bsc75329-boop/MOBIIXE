# CLAUDE.md

This file provides guidance for AI assistants working with the MOBIIXE (모빅셀 코드) repository.

## Project Overview

MOBIIXE (모빅셀 코드) is an Arduino-based motor control project. L298N 모터 드라이버를 사용하여 DC 모터를 제어합니다.

## Repository Structure

```
MOBIIXE/
├── .git/
├── motor_control/
│   ├── motor_control.ino   # 아두이노 모터 제어 스케치
│   └── HARDWARE.md         # 하드웨어 구성도 및 배선 안내
├── CLAUDE.md               # AI 어시스턴트 가이드 (이 파일)
├── GETTING_STARTED.md      # 깃허브 초보자용 사용 가이드
└── README.md               # 프로젝트 설명
```

## Current State

- **Status**: Arduino 모터 제어 코드 구현됨
- **Language/Framework**: Arduino (C/C++)
- **Build System**: Arduino IDE 또는 Arduino CLI
- **Tests**: Not yet configured
- **CI/CD**: Not yet configured
- **Linting/Formatting**: Not yet configured

## Git Workflow

- **Primary branch**: `master`
- **Remote**: `origin`
- Commit messages should be clear and descriptive
- Use conventional commit style when possible (e.g., `feat:`, `fix:`, `docs:`, `chore:`)

## Development Guidelines

When setting up this project, follow these conventions:

### General

- Keep the README.md updated as the project evolves
- Add a `.gitignore` appropriate for the chosen technology stack
- Include environment variable templates (`.env.example`) rather than committing secrets
- Never commit credentials, API keys, or sensitive configuration

### Code Comments

- **All code comments must be written in Korean (한글)**
- 주석은 명확하고 간결하게 작성할 것
- 함수, 클래스, 모듈 수준의 설명도 한글로 작성

### Code Quality

- Set up linting and formatting tools appropriate for the chosen language
- Configure pre-commit hooks for automated code quality checks
- Write tests alongside new features

### Documentation

- Update this CLAUDE.md as the project structure and conventions solidify
- Document architectural decisions in code comments or dedicated docs
- Keep dependency lists and build instructions current
