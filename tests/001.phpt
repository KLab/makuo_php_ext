--TEST--
Check for makuo presence
--SKIPIF--
<?php if (!extension_loaded("makuo")) print "skip"; ?>
--FILE--
<?php 
echo "makuo extension is available";
?>
--EXPECT--
makuo extension is available
