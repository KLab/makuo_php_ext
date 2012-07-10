--TEST--
makuo members
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

$msync_status = `msync --status`;
$msync_ary = explode("\n", trim($msync_status));
$msync = array();
for ($i=0; $i<count($msync_ary); $i++) {
  $v = split(":", $msync_ary[$i]);
  $msync[$v[0]] = trim($v[1]);
}
$status = $makuo->status();

// Diffent from msync to makuosan PHP module. 
$test_ok = true;
$max = count($msync);
if ($max != count($status)) {
  $test_ok = false;
}
foreach (array_keys($msync) as $key) {
  if ($msync[$key] != $status[$key]) {
     $test_ok = false;
     break;
  }
}
var_dump($test_ok);

$makuo->close();
?>
--EXPECT--
bool(true)
