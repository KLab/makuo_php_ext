--TEST--
makuo check
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

$check = $makuo->check();

var_dump(is_array($check));
$makuo->close();
?>
--EXPECT--
bool(true)
