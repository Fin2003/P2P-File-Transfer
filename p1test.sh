#bash
for dir in testcase/test{1..10}; do
    (cd $dir && ./run_test.sh)
done