--TEST--
makuo send
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;

var_dump($makuo->send("", array("recursive" => true, "dryrun" => true, "delete" => true)));

var_dump($makuo->send("lrfkqyuqfjkxyqvnrtysfrzrmzlygfve", array("recursive" => true, "dryrun" => true)));
var_dump($makuo->error);

$long_file_name = "";
for ($i = 0; $i < 10000; ++$i) $long_file_name .= "A";
var_dump($makuo->send($long_file_name));
var_dump($makuo->error);

$makuo->close();
?>
--EXPECT--
bool(true)
bool(false)
string(67) "error: No such file or directory lrfkqyuqfjkxyqvnrtysfrzrmzlygfve
"
bool(false)
string(18) "Too long parameter"
