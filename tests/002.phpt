--TEST--
makuo connect
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
$makuo = new Makuosan();
if (!$makuo->connect_tcp("127.0.0.1", 5000)) die;
echo "connection succeeded.";
$makuo->close();
?>
--EXPECT--
connection succeeded.
