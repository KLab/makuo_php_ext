--TEST--
makuo sync
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

var_dump($makuo->sync("", array("recursive" => true, "dryrun" => true)));

// check robustness
var_dump(@$makuo->sync());
var_dump(@$makuo->sync(1, array("recursive" => true, "dryrun" => true)));
var_dump(@$makuo->sync(42, 42));

$makuo->close();
?>
--EXPECT--
bool(true)
bool(false)
bool(false)
bool(false)
