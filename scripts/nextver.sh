set -euo pipefail
script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $script_dir/..
if [[ $(git diff HEAD --stat) != '' ]]; then
  echo "Dirty tree. Commit first please."
  exit 1
fi
next_ver=`cat ./src/rose_version.txt | awk -F. -v OFS=. '{$NF=$NF+1;print}'`
echo $next_ver > ./src/rose_version.txt
make clean && make -j all test
echo "Running bench..."
bench=`./rose-$next_ver bench | grep "bench results:" -A 3`
git add ./src/rose_version.txt
git commit -m "[$next_ver]" -m "$bench" -e
git push
popd
