Logging
-------

Logging can be configured within the config file using the *logging* root element. See the example below.

The default log level can be set using logging.level element (*level*: <level>) with level being one of (*trace*, *debug*, *info*, *warn*, *err*, *critical*, *off*).

### Formatters

For the formatter pattern flags see the [spdlog documentation](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting)

### Sinks

#### Formatter

Each sink may reference one optional Formatter (*formatter* : <formatter_id>).
A sink may have a log level (*level*: <level>) with level being one of (*trace*, *debug*, *info*, *warn*, *err*, *critical*, *off*)

#### Types

Valid sink types (*type*: <type_str>) are:

- console,
- file
- file-rotating
- file-daily

In case of file logger a path (*path* : <path>) is mandatory.

### Loggers

A logger may reference several sinks (*sinks* : [<list_of_sink_ids>]), defaults to console sink.
A logger may employ a filter-regex which must be contained (not matched).
A logger may have a log level (*level*: <level>) with level begin one of (*trace*, *debug*, *info*, *warn*, *err*, *critical*, *off*)

### Example

```yaml
logging:                       # logging definition

  level: debug
  formatters:
    CUSTOM_FORMATTER:
      format: ">>> %H:%M:%S %z %v" # custom format
  sinks:                       # sinks section
    CONSOLE_SINK:              # id of the sink
      type: "console"          # type, one of console, file, file-rotating, file-daily
      formatter: CUSTOM_FORMATTER  # set custom format for sink
      level: trace
    FILE_SINK:
      type: "file-rotating"    # in case of file, path is needed
      path: "logs/chord0.log"  # path to file, will be created if not exists

  loggers:                     # loggers section
    CHORD_LOG:                 # id of logger
      sinks: [FILE_SINK]       # list of sink references
      filter: "^chord[.](?!fs)"# filter
      level: warn
    CHORD_FS_LOG:
      sinks: [CONSOLE_SINK]
      filter: "^chord[.]fs"
```
