--TEST--
makuo exclude
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
function mysort($a) { sort($a); return $a; }
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

var_dump(mysort($makuo->exclude_list()));

$makuo->exclude_add(array("a.txt"));
var_dump(mysort($makuo->exclude_list()));

$makuo->exclude_add(array("b.txt"));
var_dump(mysort($makuo->exclude_list()));

$makuo->exclude_del("a.txt");
var_dump(mysort($makuo->exclude_list()));

$arr = array();
for ($i = 0; $i < 10; ++$i) {
    array_push($arr, "$i.txt");
}
$makuo->exclude_add($arr);
var_dump(mysort($makuo->exclude_list()));

$makuo->exclude_clear();
var_dump(mysort($makuo->exclude_list()));

$makuo->close();
?>
--EXPECT--
array(0) {
}
array(1) {
  [0]=>
  string(5) "a.txt"
}
array(2) {
  [0]=>
  string(5) "a.txt"
  [1]=>
  string(5) "b.txt"
}
array(1) {
  [0]=>
  string(5) "b.txt"
}
array(11) {
  [0]=>
  string(5) "0.txt"
  [1]=>
  string(5) "1.txt"
  [2]=>
  string(5) "2.txt"
  [3]=>
  string(5) "3.txt"
  [4]=>
  string(5) "4.txt"
  [5]=>
  string(5) "5.txt"
  [6]=>
  string(5) "6.txt"
  [7]=>
  string(5) "7.txt"
  [8]=>
  string(5) "8.txt"
  [9]=>
  string(5) "9.txt"
  [10]=>
  string(5) "b.txt"
}
array(0) {
}
