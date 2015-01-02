(ns sse-server.core
  (:require [org.httpkit.server :refer [run-server with-channel on-close send! close]]
            [org.httpkit.timer :refer [schedule-task]]))

(def channel-hub (atom {}))

(defn broadcast [message]
  (doseq [channel (keys @channel-hub)]
    (send! channel message false)))

(defn handle-cors [_]
  {:status 204
   :headers {"Access-Control-Allow-Origin" "*"
             "Connection" "close"}})

(defn handle-404 []
  {:status 404
   :headers {"Content-Type" "text/plain"}
   :body "File not found"})

(defn handle-connections [_]
  {:status 200
   :headers {"Content-Type" "text/plain"
             "Access-Control-Allow-Origin" "*"
             "Cache-Control" "no-cache"
             "Connection" "close"}
   :body (str (count @channel-hub))})

(defn handle-sse [req]
  (with-channel req channel
    (send! channel {:status 200
                    :headers {"Content-Type" "text/event-stream"
                              "Access-Control-Allow-Origin" "*"
                              "Cache-Control" "no-cache"
                              "Connection" "keep-alive"}
                    :body ":ok\n\n"} false)
    (swap! channel-hub assoc channel req)
    (on-close channel (fn [_]
                        (swap! channel-hub dissoc channel)))))

(defn handle-broadcast [req]
  (let [body (:body req)]
    (if (not (nil? body))
      (do
        (broadcast (str "data: " (slurp body) "\n\n"))
        {:status 200
         :headers {"Content-Type" "text/plain"}
         :body (str "Broadcasted to " (count @channel-hub) " clients")})
      {:status 400
       :headers {"Content-Type" "text/plain"}
       :body "Empty message"})))

(def uri-handlers
  {{:uri "/connections" :method :get} handle-connections
   {:uri "/connections" :method :options} handle-cors
   {:uri "/sse" :method :get} handle-sse
   {:uri "/sse" :method :options} handle-cors
   {:uri "/broadcast" :method :post} handle-broadcast})

(defn app [req]
  (let [request-id {:uri (:uri req) :method (:request-method req)}]
    (if (contains? uri-handlers request-id)
      ((uri-handlers request-id) req)
      (handle-404))))

(defn notify-channels []
  (let [time (System/currentTimeMillis)]
    (broadcast (str "data: " time "\n\n"))))

(defn start-timer []
  (schedule-task 1000
                 (notify-channels)
                 (start-timer)))

(defn parse-port [args]
  (if (= (count args) 0)
    1942
    (Integer. (first args))))

(defn -main
  [& args]
  (let [port (parse-port args)]
    (run-server app {:port port :thread 8 :queue-size 300000})
    (start-timer)
    (print (str "Listening on http://127.0.0.1:" port))))
