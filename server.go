package main

import (
    "log"
    "net/http"
)

func main() {
    http.HandleFunc(
        "/",
        func(w http.ResponseWriter, r *http.Request) {
            http.ServeFile(w, r, "build_out/"+r.URL.Path[1:])
        })

    log.Fatal(http.ListenAndServe(":8081", nil))
}
