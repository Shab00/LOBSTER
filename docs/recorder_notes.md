# Recorder Resilience Notes

## Purpose
These notes define the behavior of the data recorder
(scripts/record_stream.py) before it is built. The recorder must
run unattended for multiple days, so it needs to handle
disconnects, file rotation, and metadata logging.

## 1. Auto-reconnect
- On WebSocket disconnect, wait 5 seconds, then reconnect.
- If reconnect fails, double the wait time, up to 60 seconds max.
- Retry forever — never give up silently.
- Log every reconnect event to recording_meta.json.

## 2. Daily file rotation
- Check the current date on each message received.
- If the date has changed since the last message:
    - Close the current file.
    - Open a new file with the new date in the name.
- File naming:
    - depth_YYYYMMDD.jsonl
    - trades_YYYYMMDD.jsonl
- Never append across a midnight boundary.

## 3. Metadata file
- Write recording_meta.json immediately on startup with:
    - start_time
    - symbol
    - streams (depth, trades)
    - status: "running"
- Append an event on every reconnect:
    - time
    - event: "reconnect"
- On clean exit (Ctrl+C), update:
    - status: "stopped"
    - end_time
- If the process crashes, the meta file still shows when recording
  started and what was being captured.
