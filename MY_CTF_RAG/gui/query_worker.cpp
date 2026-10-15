#include "query_worker.h"

QueryResultPtr run_query_chain(
    const QueryRequest& request,
    const std::function<void()>& on_generation_started) {
    auto result = std::make_shared<QueryResult>();

    if (request.engine == nullptr)
        return QueryResultPtr(std::move(result));

    // Phase 1: retrieval. Fast, and the UI gets it as soon as this
    // returns -- the whole point of the two-phase contract.
    result->outcome = request.engine->retrieve(request.query);

    // Phase 2: the grounded answer. generate() itself skips when there
    // is nothing to ground, so the callback and the call are both
    // guarded only by withGeneration.
    if (request.withGeneration && !result->outcome.hits.empty()) {
        if (on_generation_started)
            on_generation_started();
        request.engine->generate(request.query, result->outcome);
    }

    // Drop the caller's write access: the UI only ever renders this.
    return QueryResultPtr(std::move(result));
}
