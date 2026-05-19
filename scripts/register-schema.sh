#!/usr/bin/env sh
set -eu

SCHEMA_REGISTRY_URL="${SCHEMA_REGISTRY_URL:-http://$SCHEMA_REGISTRY_HOST_NAME:8081}"
SUBJECT="${SCHEMA_SUBJECT}"
SCHEMA_FILE="${SCHEMA_FILE:-/schemas/frame_event.proto}"

if [ ! -f "$SCHEMA_FILE" ]; then
  echo "Schema file not found: $SCHEMA_FILE" >&2
  exit 1
fi

echo "Waiting for Schema Registry at $SCHEMA_REGISTRY_URL ..."
for _ in $(seq 1 60); do
  if curl -fsS "$SCHEMA_REGISTRY_URL/subjects" >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

if ! curl -fsS "$SCHEMA_REGISTRY_URL/subjects" >/dev/null 2>&1; then
  echo "Schema Registry is not reachable" >&2
  exit 1
fi

SCHEMA_CONTENT=$(sed ':a;N;$!ba;s/\\/\\\\/g;s/"/\\"/g;s/\n/\\n/g' "$SCHEMA_FILE")
PAYLOAD=$(printf '{"schemaType":"PROTOBUF","schema":"%s"}' "$SCHEMA_CONTENT")

RESPONSE=$(curl -fsS -X POST \
  -H 'Content-Type: application/vnd.schemaregistry.v1+json' \
  --data "$PAYLOAD" \
  "$SCHEMA_REGISTRY_URL/subjects/$SUBJECT/versions")

echo "Registered schema for subject '$SUBJECT': $RESPONSE"
