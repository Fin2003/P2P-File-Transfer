#bash
for dir in testcase/test{11..16}; do
    (cd $dir && ./run_test.sh)
done