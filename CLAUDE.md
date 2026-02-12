# CLAUDE.md

This file provides guidance for AI assistants working with the MOBIIXE (모빅셀 코드) repository.

## Project Overview

MOBIIXE is a newly initialized project. The repository is in its early stages with no source code, build system, or configuration files established yet.

## Repository Structure

```
MOBIIXE/
├── .git/
├── CLAUDE.md        # This file - AI assistant guidance
└── README.md        # Project description
```

## Current State

- **Status**: Initial scaffold - no implementation exists yet
- **Language/Framework**: Not yet determined
- **Build System**: Not yet configured
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
