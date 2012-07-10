--TEST--
makuo members
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

$members = $makuo->members();

var_dump(is_array($members));
$makuo->close();
?>
--EXPECT--
bool(true)
