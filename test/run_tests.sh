#!/bin/bash
PROG=../myPreCompiler   # percorso al tuo eseguibile
OK=0
FAIL=0

function run {
  name=$1
  echo -n "Test $name: "
  "$PROG" -i "$name.c" > "$name.out" 2> "$name.err"
  diff -u "$name.expected" "$name.out" >/dev/null
  d1=$?
  # se ho un .stats atteso, controllo anche stderr
  if [[ -f "$name.stats" ]]; then
    "$PROG" -v -i "$name.c" > /dev/null 2> "$name.stats.out"
    diff -u "$name.stats" "$name.stats.out" >/dev/null
    d2=$?
  else
    d2=0
  fi

  if [[ $d1 -eq 0 && $d2 -eq 0 ]]; then
    echo "OK"
    ((OK++))
  else
    echo "FAIL"
    echo "  diff del risultato:"
    diff -u "$name.expected" "$name.out"
    if [[ -f "$name.stats" ]]; then
      echo "  diff delle stats:"
      diff -u "$name.stats" "$name.stats.out"
    fi
    ((FAIL++))
  fi
}

cd "$(dirname "$0")"
# per ogni test
for t in include comments identifiers combined; do
  run "$t"
done

echo
echo "Passed: $OK  Failed: $FAIL"
exit $FAIL
