-module(eventsource_app).
-behaviour(application).

-export([start/2]).
-export([stop/1]).

start(_Type, _Args) ->
    Dispatch = cowboy_router:compile([
        {'_', [
            {"/sse", eventsource_emitter, []},
            {"/connections", connections_handler, []}
        ]}
    ]),
    cowboy:start_http(sse_handler, 100, [
            {port, 1942},
            {max_connections, infinity}
        ],
        [{env, [{dispatch, Dispatch}]}]
    ),
    eventsource_sup:start_link().

stop(_State) ->
    ok.
