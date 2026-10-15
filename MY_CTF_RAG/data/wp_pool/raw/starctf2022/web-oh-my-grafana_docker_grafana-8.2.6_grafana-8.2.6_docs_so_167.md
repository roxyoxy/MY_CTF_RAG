# web-oh-my-grafana_docker_grafana-8.2.6_grafana-8.2.6_docs_sources_enterprise_usage-insights__index

+++
title = "Usage insights"
description = "Understand how your Grafana instance is used"
keywords = ["grafana", "usage-insights", "enterprise"]
aliases = ["/docs/grafana/latest/enterprise/usage-insights/"]
weight = 200
+++

# Usage insights

Usage insights allow you to have a better understanding of how your Grafana instance is used.

The usage insights feature collects a number of aggregated data and stores them in the database:

- Dashboard views (aggregated and per user)
- Data source errors
- Data source queries

These aggregated data give you access to several features:

- [Dashboard and data source insights]({{< relref "dashboard-datasource-insights.md" >}})
- [Presence indicator]({{< relref "presence-indicator.md" >}})
- [Sort dashboards by using insights data]({{< relref "improved-search.md" >}})

This feature also generates detailed logs that can be exported to Loki. Refer to [Export logs of usage insights]({{< relref "export-logs.md" >}}).
