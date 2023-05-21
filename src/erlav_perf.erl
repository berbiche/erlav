-module(erlav_perf).

-export([ 
    erlav_perf_tst/1, 
    erlav_perf_tst2/1, 
    erlav_perf_string/2, 
    erlav_perf_integer/1,
    erlav_perf_strings/3
]).

erlav_perf_tst(Num) ->
    {ok, SchemaJSON1} = file:read_file("test/opnrtb_test1.avsc"),
    Encoder  = avro:make_simple_encoder(SchemaJSON1, []),
    _Decoder  = avro:make_simple_decoder(SchemaJSON1, []),
    SchemaId = erlav_nif:create_encoder(<<"test/opnrtb_test1.avsc">>),
    L = lists:seq(1, Num),
    {ok, [Term1]} = file:consult("test/opnrtb_perf.data"),

    io:format("Started ..... ~n", []),

    T1 = erlang:system_time(microsecond),
    lists:foreach(fun(_) ->
        iolist_to_binary(Encoder(Term1))
    end, L),
    Total1 = erlang:system_time(microsecond) - T1,
    io:format("Erlavro encoding time: ~p microseconds ~n", [T1]),

    T2 = erlang:system_time(microsecond),

    lists:foreach(fun(_) ->
        erlav_nif:do_encode(SchemaId, Term1)
    end, L),
    Total2 = erlang:system_time(microsecond) - T2,
    io:format("Erlav encoding time: ~p microseconds ~n", [T2]),

    {Total1/Num, Total2/Num}.

erlav_perf_tst2(Num) ->
    {ok, SchemaJSON1} = file:read_file("test/opnrtb_test1.avsc"),
    Encoder  = avro:make_simple_encoder(SchemaJSON1, []),
    _Decoder  = avro:make_simple_decoder(SchemaJSON1, []),
    SchemaId = erlav_nif:create_encoder(<<"test/opnrtb_test1.avsc">>),
    {ok, [Term1]} = file:consult("test/opnrtb_perf.data"),
    Terms = [ randomize(Term1) || _ <- lists:seq(1, Num)],

    io:format("Started ..... ~n", []),

    T1 = erlang:system_time(microsecond),
    lists:foreach(fun(Term1r) ->
        iolist_to_binary(Encoder(Term1r))
                  end, Terms),
    Total1 = erlang:system_time(microsecond) - T1,
    io:format("Erlavro encoding time: ~p microseconds ~n", [T1]),

    T2 = erlang:system_time(microsecond),

    lists:foreach(fun(Term1r) ->
        erlav_nif:do_encode(SchemaId, Term1r)
                  end, Terms),
    Total2 = erlang:system_time(microsecond) - T2,
    io:format("Erlav encoding time: ~p microseconds ~n", [T2]),

    {Total1/Num, Total2/Num}.

randomize(Map) when is_map(Map) ->
    maps:filtermap(fun
            (_, V) when is_binary(V)  -> {true, base64:encode(crypto:strong_rand_bytes(100))}; 
            (_, V) when is_integer(V) -> {true, rand:uniform(999999)};
            (_, V) when is_map(V)     -> {true, randomize(V)};
            (_, V) when is_list(V)    -> {true, [ randomize(Vi) || Vi <- V]};
            (_, _) -> true
    end, Map);
randomize(S) when is_binary(S) -> base64:encode(crypto:strong_rand_bytes(100));
randomize(I) when is_integer(I) -> rand:uniform(9999999);
randomize(V) -> V.


erlav_perf_string(Num, StrLen) ->
    {ok, SchemaJSON1} = file:read_file("test/string.avsc"),
    Encoder  = avro:make_simple_encoder(SchemaJSON1, []),
    _Decoder  = avro:make_simple_decoder(SchemaJSON1, []),
    SchemaId = erlav_nif:create_encoder(<<"test/string.avsc">>),

    [St1 | _] = Strings = [ base64:encode(crypto:strong_rand_bytes(StrLen)) || _ <- lists:seq(1, Num)],

    io:format("Started ..... ~n", []),

    T1 = erlang:system_time(microsecond),
    lists:foreach(fun(S) ->
        iolist_to_binary(Encoder(#{ <<"stringField">> => S }))
    end, Strings),
    Total1 = erlang:system_time(microsecond) - T1,
    io:format("Erlavro encoding time: ~p microseconds ~n", [T1]),

    T2 = erlang:system_time(microsecond),

    lists:foreach(fun(S) ->
        erlav_nif:do_encode(SchemaId, #{ <<"stringField">> => S })
    end, Strings),
    Total2 = erlang:system_time(microsecond) - T2,
    io:format("Erlav encoding time: ~p microseconds ~n", [T2]),
    
    TestMap = #{ <<"stringField">> => St1 },
    RetAvro1 = erlav_nif:do_encode(SchemaId, TestMap),
    RetAvro2 = iolist_to_binary(Encoder(TestMap)),
    IsSame = RetAvro1 =:= RetAvro2,
    io:format("Same ret: ~p ~n ~p ~n ~p ~n", [IsSame, RetAvro2, RetAvro1]),

    {Total1/Num, Total2/Num}.

erlav_perf_integer(Num) ->
    {ok, SchemaJSON1} = file:read_file("test/integer.avsc"),
    Encoder  = avro:make_simple_encoder(SchemaJSON1, []),
    _Decoder  = avro:make_simple_decoder(SchemaJSON1, []),
    SchemaId = erlav_nif:create_encoder(<<"test/integer.avsc">>),

    Ints = [ rand:uniform(9999999) || _ <- lists:seq(1, Num)],

    io:format("Started ..... ~n", []),

    T1 = erlang:system_time(microsecond),
    lists:foreach(fun(S) ->
        iolist_to_binary(Encoder(#{ <<"intField">> => S }))
    end, Ints),
    Total1 = erlang:system_time(microsecond) - T1,
    io:format("Erlavro encoding time: ~p microseconds ~n", [T1]),

    T2 = erlang:system_time(microsecond),

    lists:foreach(fun(S) ->
        erlav_nif:do_encode(SchemaId, #{ <<"intField">> => S })
    end, Ints),
    Total2 = erlang:system_time(microsecond) - T2,
    io:format("Erlav encoding time: ~p microseconds ~n", [T2]),

    {Total1/Num, Total2/Num}.

erlav_perf_strings(Num, StrLen, Type) ->
    Schema = case Type of
        null -> "test/perfstr.avsc";
        _ -> "test/perfstr_null.avsc"
    end,
    {ok, SchemaJSON1} = file:read_file(Schema),
    Encoder  = avro:make_simple_encoder(SchemaJSON1, []),
    _Decoder  = avro:make_simple_decoder(SchemaJSON1, []),
    SchemaId = erlav_nif:create_encoder(Schema),

    Strings = [ [base64:encode(crypto:strong_rand_bytes(StrLen)) || _ <- lists:seq(1, 6)] || _ <- lists:seq(1, Num)],

    io:format("Started ..... ~n", []),

    T1 = erlang:system_time(microsecond),
    lists:foreach(fun([S1,S2,S3,S4,S5,S6]) ->
        iolist_to_binary(Encoder(#{ 
            <<"stringField1">> => S1,
            <<"stringField2">> => S2,
            <<"stringField3">> => S3,
            <<"stringField4">> => S4,
            <<"stringField5">> => S5,
            <<"stringField6">> => S6
        }))
    end, Strings),
    Total1 = erlang:system_time(microsecond) - T1,
    io:format("Erlavro encoding time: ~p microseconds ~n", [T1]),

    T2 = erlang:system_time(microsecond),

    lists:foreach(fun([S1,S2,S3,S4,S5,S6]) ->
        erlav_nif:do_encode(SchemaId, #{ 
            <<"stringField1">> => S1,
            <<"stringField2">> => S2,
            <<"stringField3">> => S3,
            <<"stringField4">> => S4,
            <<"stringField5">> => S5,
            <<"stringField6">> => S6
        })
    end, Strings),
    Total2 = erlang:system_time(microsecond) - T2,
    io:format("Erlav encoding time: ~p microseconds ~n", [T2]),

    [ [ St1, St2, St3, St4, St5, St6 ] | _ ] = Strings,
    TestMap = #{ 
        <<"stringField1">> => St1,
        <<"stringField2">> => St2,
        <<"stringField3">> => St3,
        <<"stringField4">> => St4,
        <<"stringField5">> => St5,
        <<"stringField6">> => St6
    },
    RetAvro1 = erlav_nif:do_encode(SchemaId, TestMap),
    RetAvro2 = iolist_to_binary(Encoder(TestMap)),
    IsSame = RetAvro1 =:= RetAvro2,
    io:format("Test Term: ~p ~n", [TestMap]),
    io:format("Same ret: ~p ~n ~p ~n ~p ~n", [IsSame, RetAvro2, RetAvro1]),


    {Total1/Num, Total2/Num}.
