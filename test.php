<?php
define('PRINT_RESULT', false);

function TestStatus($makuo) {
    echo "testing \$makuo->status()...\n";
    //Array
    //(
    //    [process] => 21995
    //    [version] => 1.2.1
    //    [basedir] => /home/kishimoto-k/makuo-test/base/
    //    [mfalloc] => 2
    //    [command] => 0
    //    [send op] => 0
    //    [recv op] => 0
    //)
    $res = $makuo->status();
    if (PRINT_RESULT){ print_r($res); }

    assert(is_array($res));
    foreach (array("process", "version", "basedir",
                   "command", "send op", "recv op") as $key) {
        assert(array_key_exists($key, $res));
    }
}

function TestMembers($makuo) {
    echo "testing \$makuo->members()...\n";
    //Array
    //(
    //    [0] => 10.10.2.27
    //)
    $res = $makuo->members();
    if (PRINT_RESULT){ print_r($res); }

    assert(is_array($res));
    foreach ($res as $key => $value) {
        assert(is_integer($key));
        assert(substr_count($value, ".") == 3);  // $value = x.x.x.x
    }
}

function TestCheck($makuo) {
    echo "testing \$makuo->check()...\n";
    //Array
    //(
    //    [0] => error: No such file or directory sag15:sag14.txt
    //    [1] => error: No such file or directory sag15:sag14-2.txt
    //)
    $res = $makuo->check();
    if (PRINT_RESULT){ print_r($res); }

    assert(is_array($res));
    foreach ($res as $key => $value) {
        assert(is_integer($key));
    }
}

function TestSend($makuo) {
    echo "testing \$makuo->send()...\n";

    assert($makuo->send("", array("recursive" => true, "dryrun" => true, "delete" => true)));

    assert(!$makuo->send("foo", array("recursive" => true, "dryrun" => true, "bar")));
    assert($makuo->error === "error: No such file or directory foo\r\n");

    // check robustness
    //assert(!$makuo->send(1, 1));

    $str = "A";
    while (strlen($str) < 10000) { $str .= $str; }
    assert($makuo->send($str) == false);
    assert($makuo->error === "Too long parameter");

    $str = "A";
    while (strlen($str) < 1024) { $str .= $str; }
    assert($makuo->send($str) == false);
    assert($makuo->error === "Too long parameter");
}

function TestSync($makuo) {
    echo "testing \$makuo->sync()...\n";
    $makuo->sync("", array("recursive" => true, "dryrun" => true));

    // check robustness
    @$makuo->sync();
    @$makuo->sync(1, array("recursive" => true, "dryrun" => true));
    @$makuo->sync(42, 42);
}

function TestExclude($makuo) {
    echo "testing \$makuo->exclude_*()...\n";

    $l = $makuo->exclude_list();
    assert(is_array($l));
    assert(empty($l));

    $makuo->exclude_add(array("a.txt"));
    $l = $makuo->exclude_list();

    assert(!empty($l));
    assert(count($l) == 1);
    assert($l[0] == 'a.txt');

    $makuo->exclude_add(array("b.txt"));
    $l = $makuo->exclude_list();
    assert(count($l) == 2);

    $makuo->exclude_del("a.txt");
    $l = $makuo->exclude_list();
    assert(count($l) == 1);
    assert($l[0] == 'b.txt');

    $arr = array();
    for ($i = 0; $i < 10; ++$i) {
        array_push($arr, "$i.txt");
    }
    $makuo->exclude_add($arr);
    $l = $makuo->exclude_list();
    assert(count($l) == 11);

    $makuo->exclude_clear();
    $l = $makuo->exclude_list();
    assert(is_array($l));
    assert(empty($l));
}

function RunAllTests() {
    $makuo = new Makuosan(array("rcv_timeout" => 5));

    if (!$makuo->connect_tcp("127.0.0.1", 5000, 1)) {
        echo "could not connect.\n";
        echo "ErrorMessage: " . $makuo->error . "\n";
        return;
    }

/*     if (!$makuo->connect_unix("/Users/keisuke/klab/php-module/makuo.socket", 1)) { */
/*     if (!$makuo->connect_unix("/home/kishimoto-k/makuo-test/makuosan.sock")) { */
/*         echo "could not connect.\n"; */
/*         echo "ErrorMessage: " . $makuo->error . "\n"; */
/*         return; */
/*     } */

    TestStatus($makuo);
    TestMembers($makuo);
    TestCheck($makuo);
    TestSend($makuo);
    TestSync($makuo);
    TestExclude($makuo);

    $makuo->close();
}

RunAllTests();
?>
