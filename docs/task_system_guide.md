# Task Master System Guide

## Understanding Task Organization

The project uses Task Master to manage tasks. Here's how the file organization works:

### Essential Files (DO NOT DELETE)

* `tasks/tasks.json` - The primary database file that stores all task information
* `tasks/task_001.txt` (etc.) - Individual task files used by Task Master

These files are used directly by Task Master and should not be deleted or modified outside of Task Master commands.

### Documentation Files (Human-Readable)

* `docs/progress/task_dashboard.md` - A human-readable dashboard generated from Task Master data
* `docs/tasks/*.md` - Individual task documentation files (optional)
* `docs/planning/product_requirements.md` - Project requirements document

### Key Commands

To interact with Task Master, use these commands in the terminal:

* `npx task-master list` - Display all tasks with their status
* `npx task-master show 1` - Show details for task #1
* `npx task-master set-status --id=1 --status=done` - Mark task #1 as complete
* `npx task-master next` - Show the recommended next task to work on
* `npx task-master expand --id=1` - Break down task #1 into subtasks

### Dashboard Generation

The terminal output of `npx task-master list` shows a nicely formatted dashboard with:
* Task progress overview
* Subtask progress 
* Priority breakdown
* Dependency status
* Next task recommendation
* Table of all tasks

You can regenerate the markdown dashboard file by running:
* `./scripts/task_dashboard.ps1`

This creates a markdown version of the dashboard that can be viewed in the docs folder.

## In Summary

1. The `tasks/` directory contains the actual Task Master data files (essential)
2. The `docs/` directory contains human-readable documentation (can be regenerated)
3. Use Task Master commands in the terminal to view and manage tasks
4. The dashboard script helps generate markdown versions of the task status 