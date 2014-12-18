-module(eventsource_emitter).
-behaviour(cowboy_loop_handler).

-export([init/3]).
-export([info/3]).
-export([terminate/3]).

-record(state, {}).

init({_Any, http}, Req, _Opts) ->
    Headers = [
        {<<"content-type">>, <<"text/event-stream">>},
        {<<"cache-control">>, <<"no-cache">>},
        {<<"connection">>, <<"keep-alive">>}
    ],
    {ok, Req2} = cowboy_req:chunked_reply(200, Headers, Req),
    ok = cowboy_req:chunk([":ok", "\n\n"], Req2),
    timer:apply_interval(1000, erlang, apply, [ fun sendMessage/1, [self()] ]),
    {loop, Req2, #state{}}.

info({message, Msg}, Req, State) ->
    ok = cowboy_req:chunk(Msg, Req),
    {loop, Req, State}.

terminate(_Reason, _Req, _State) ->
    ok.

sendMessage(Pid) ->
    Pid ! {message, ["data: ", microtime(), "\n\n"]}.

microtime() ->
    {Mega, Sec, Micro} = os:timestamp(),
    Id = round((((Mega * 1000000) + Sec) * 1000) + (Micro / 1000)),
    integer_to_list(Id).
