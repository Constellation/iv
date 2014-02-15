(function () {
  var env = jasmine.getEnv();
  env.updateInterval = 1000;
  var reporter = new jasmine.ConsoleReporter(log, function(runner) {
    var results = runner.results();
    if (results.failedCount) {
      throw new Error("spec failed");
    }
  }, true);
  env.addReporter(reporter);
  env.execute();
}());
