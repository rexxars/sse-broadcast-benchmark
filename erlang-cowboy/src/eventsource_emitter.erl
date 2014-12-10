-module(eventsource_emitter).
-behaviour(cowboy_loop_handler).

-export([init/3]).
-export([info/3]).
-export([terminate/3]).

-record(state, {}).

init({_Any, http}, Req, _Opts) ->
    timer:send_after(0, {message, [":ok", "\n\n"]}),
    Headers = [
        {<<"content-type">>, <<"text/event-stream">>},
        {<<"cache-control">>, <<"no-cache">>},
        {<<"connection">>, <<"keep-alive">>}
    ],
    {ok, Req2} = cowboy_req:chunked_reply(200, Headers, Req),
    {loop, Req2, #state{}, hibernate}.

info(shutdown, Req, State) ->
    {ok, Req, State};

info({message, Msg}, Req, State) ->
    ok = cowboy_req:chunk(Msg, Req),
    timer:send_after(1000, {message, ["data: ", id(), "\n\n"]}),
    {loop, Req, State, hibernate}.

terminate(_Reason, _Req, _State) ->
    ok.

id() ->
    {Mega, Sec, _Micro} = os:timestamp(),
    Id = ((Mega * 1000000 + Sec) * 1000),
    integer_to_list(Id).
