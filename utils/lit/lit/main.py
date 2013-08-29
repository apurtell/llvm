#!/usr/bin/env python

"""
lit - LLVM Integrated Tester.

See lit.pod for more information.
"""

from __future__ import absolute_import
import math, os, platform, random, re, sys, time, threading, traceback

import lit.ProgressBar
import lit.LitConfig
import lit.Test
import lit.run
import lit.util

import lit.discovery

class TestingProgressDisplay:
    def __init__(self, opts, numTests, progressBar=None):
        self.opts = opts
        self.numTests = numTests
        self.current = None
        self.lock = threading.Lock()
        self.progressBar = progressBar
        self.completed = 0

    def update(self, test):
        # Avoid locking overhead in quiet mode
        if self.opts.quiet and not test.result.code.isFailure:
            self.completed += 1
            return

        # Output lock.
        self.lock.acquire()
        try:
            self.handleUpdate(test)
        finally:
            self.lock.release()

    def finish(self):
        if self.progressBar:
            self.progressBar.clear()
        elif self.opts.quiet:
            pass
        elif self.opts.succinct:
            sys.stdout.write('\n')

    def handleUpdate(self, test):
        self.completed += 1
        if self.progressBar:
            self.progressBar.update(float(self.completed)/self.numTests,
                                    test.getFullName())

        if self.opts.succinct and not test.result.code.isFailure:
            return

        if self.progressBar:
            self.progressBar.clear()

        print('%s: %s (%d of %d)' % (test.result.code.name, test.getFullName(),
                                     self.completed, self.numTests))

        if test.result.code.isFailure and self.opts.showOutput:
            print("%s TEST '%s' FAILED %s" % ('*'*20, test.getFullName(),
                                              '*'*20))
            print(test.result.output)
            print("*" * 20)

        sys.stdout.flush()

class TestProvider:
    def __init__(self, tests, maxTime):
        self.maxTime = maxTime
        self.iter = iter(tests)
        self.lock = threading.Lock()
        self.startTime = time.time()
        self.canceled = False

    def cancel(self):
        self.lock.acquire()
        self.canceled = True
        self.lock.release()

    def get(self):
        # Check if we have run out of time.
        if self.maxTime is not None:
            if time.time() - self.startTime > self.maxTime:
                return None

        # Otherwise take the next test.
        self.lock.acquire()
        if self.canceled:
          self.lock.release()
          return None
        for item in self.iter:
            break
        else:
            item = None
        self.lock.release()
        return item

class Tester(threading.Thread):
    def __init__(self, litConfig, provider, display):
        threading.Thread.__init__(self)
        self.litConfig = litConfig
        self.provider = provider
        self.display = display

    def run(self):
        while 1:
            item = self.provider.get()
            if item is None:
                break
            self.runTest(item)

    def runTest(self, test):
        result = None
        startTime = time.time()
        try:
            result = test.config.test_format.execute(test, self.litConfig)

            # Support deprecated result from execute() which returned the result
            # code and additional output as a tuple.
            if isinstance(result, tuple):
                code, output = result
                result = lit.Test.Result(code, output)
            elif not isinstance(result, lit.Test.Result):
                raise ValueError("unexpected result from test execution")
        except KeyboardInterrupt:
            # This is a sad hack. Unfortunately subprocess goes
            # bonkers with ctrl-c and we start forking merrily.
            print('\nCtrl-C detected, goodbye.')
            os.kill(0,9)
        except:
            if self.litConfig.debug:
                raise
            output = 'Exception during script execution:\n'
            output += traceback.format_exc()
            output += '\n'
            result = lit.Test.Result(lit.Test.UNRESOLVED, output)
        result.elapsed = time.time() - startTime

        test.setResult(result)
        self.display.update(test)

def runTests(numThreads, litConfig, provider, display):
    # If only using one testing thread, don't use threads at all; this lets us
    # profile, among other things.
    if numThreads == 1:
        t = Tester(litConfig, provider, display)
        t.run()
        return

    # Otherwise spin up the testing threads and wait for them to finish.
    testers = [Tester(litConfig, provider, display)
               for i in range(numThreads)]
    for t in testers:
        t.start()
    try:
        for t in testers:
            t.join()
    except KeyboardInterrupt:
        sys.exit(2)

def main(builtinParameters = {}):
    # Bump the GIL check interval, its more important to get any one thread to a
    # blocking operation (hopefully exec) than to try and unblock other threads.
    #
    # FIXME: This is a hack.
    sys.setcheckinterval(1000)

    global options
    from optparse import OptionParser, OptionGroup
    parser = OptionParser("usage: %prog [options] {file-or-path}")

    parser.add_option("-j", "--threads", dest="numThreads", metavar="N",
                      help="Number of testing threads",
                      type=int, action="store", default=None)
    parser.add_option("", "--config-prefix", dest="configPrefix",
                      metavar="NAME", help="Prefix for 'lit' config files",
                      action="store", default=None)
    parser.add_option("", "--param", dest="userParameters",
                      metavar="NAME=VAL",
                      help="Add 'NAME' = 'VAL' to the user defined parameters",
                      type=str, action="append", default=[])

    group = OptionGroup(parser, "Output Format")
    # FIXME: I find these names very confusing, although I like the
    # functionality.
    group.add_option("-q", "--quiet", dest="quiet",
                     help="Suppress no error output",
                     action="store_true", default=False)
    group.add_option("-s", "--succinct", dest="succinct",
                     help="Reduce amount of output",
                     action="store_true", default=False)
    group.add_option("-v", "--verbose", dest="showOutput",
                     help="Show all test output",
                     action="store_true", default=False)
    group.add_option("", "--no-progress-bar", dest="useProgressBar",
                     help="Do not use curses based progress bar",
                     action="store_false", default=True)
    parser.add_option_group(group)

    group = OptionGroup(parser, "Test Execution")
    group.add_option("", "--path", dest="path",
                     help="Additional paths to add to testing environment",
                     action="append", type=str, default=[])
    group.add_option("", "--vg", dest="useValgrind",
                     help="Run tests under valgrind",
                     action="store_true", default=False)
    group.add_option("", "--vg-leak", dest="valgrindLeakCheck",
                     help="Check for memory leaks under valgrind",
                     action="store_true", default=False)
    group.add_option("", "--vg-arg", dest="valgrindArgs", metavar="ARG",
                     help="Specify an extra argument for valgrind",
                     type=str, action="append", default=[])
    group.add_option("", "--time-tests", dest="timeTests",
                     help="Track elapsed wall time for each test",
                     action="store_true", default=False)
    group.add_option("", "--no-execute", dest="noExecute",
                     help="Don't execute any tests (assume PASS)",
                     action="store_true", default=False)
    parser.add_option_group(group)

    group = OptionGroup(parser, "Test Selection")
    group.add_option("", "--max-tests", dest="maxTests", metavar="N",
                     help="Maximum number of tests to run",
                     action="store", type=int, default=None)
    group.add_option("", "--max-time", dest="maxTime", metavar="N",
                     help="Maximum time to spend testing (in seconds)",
                     action="store", type=float, default=None)
    group.add_option("", "--shuffle", dest="shuffle",
                     help="Run tests in random order",
                     action="store_true", default=False)
    group.add_option("", "--filter", dest="filter", metavar="REGEX",
                     help=("Only run tests with paths matching the given "
                           "regular expression"),
                     action="store", default=None)
    parser.add_option_group(group)

    group = OptionGroup(parser, "Debug and Experimental Options")
    group.add_option("", "--debug", dest="debug",
                      help="Enable debugging (for 'lit' development)",
                      action="store_true", default=False)
    group.add_option("", "--show-suites", dest="showSuites",
                      help="Show discovered test suites",
                      action="store_true", default=False)
    group.add_option("", "--show-tests", dest="showTests",
                      help="Show all discovered tests",
                      action="store_true", default=False)
    parser.add_option_group(group)

    (opts, args) = parser.parse_args()

    if not args:
        parser.error('No inputs specified')

    if opts.numThreads is None:
# Python <2.5 has a race condition causing lit to always fail with numThreads>1
# http://bugs.python.org/issue1731717
# I haven't seen this bug occur with 2.5.2 and later, so only enable multiple
# threads by default there.
       if sys.hexversion >= 0x2050200:
               opts.numThreads = lit.util.detectCPUs()
       else:
               opts.numThreads = 1

    inputs = args

    # Create the user defined parameters.
    userParams = dict(builtinParameters)
    for entry in opts.userParameters:
        if '=' not in entry:
            name,val = entry,''
        else:
            name,val = entry.split('=', 1)
        userParams[name] = val

    # Create the global config object.
    litConfig = lit.LitConfig.LitConfig(
        progname = os.path.basename(sys.argv[0]),
        path = opts.path,
        quiet = opts.quiet,
        useValgrind = opts.useValgrind,
        valgrindLeakCheck = opts.valgrindLeakCheck,
        valgrindArgs = opts.valgrindArgs,
        noExecute = opts.noExecute,
        debug = opts.debug,
        isWindows = (platform.system()=='Windows'),
        params = userParams,
        config_prefix = opts.configPrefix)

    # Perform test discovery.
    run = lit.run.Run(litConfig,
                      lit.discovery.find_tests_for_inputs(litConfig, inputs))

    if opts.showSuites or opts.showTests:
        # Aggregate the tests by suite.
        suitesAndTests = {}
        for t in run.tests:
            if t.suite not in suitesAndTests:
                suitesAndTests[t.suite] = []
            suitesAndTests[t.suite].append(t)
        suitesAndTests = list(suitesAndTests.items())
        suitesAndTests.sort(key = lambda item: item[0].name)

        # Show the suites, if requested.
        if opts.showSuites:
            print('-- Test Suites --')
            for ts,ts_tests in suitesAndTests:
                print('  %s - %d tests' %(ts.name, len(ts_tests)))
                print('    Source Root: %s' % ts.source_root)
                print('    Exec Root  : %s' % ts.exec_root)

        # Show the tests, if requested.
        if opts.showTests:
            print('-- Available Tests --')
            for ts,ts_tests in suitesAndTests:
                ts_tests.sort(key = lambda test: test.path_in_suite)
                for test in ts_tests:
                    print('  %s' % (test.getFullName(),))

        # Exit.
        sys.exit(0)

    # Select and order the tests.
    numTotalTests = len(run.tests)

    # First, select based on the filter expression if given.
    if opts.filter:
        try:
            rex = re.compile(opts.filter)
        except:
            parser.error("invalid regular expression for --filter: %r" % (
                    opts.filter))
        run.tests = [t for t in run.tests
                     if rex.search(t.getFullName())]

    # Then select the order.
    if opts.shuffle:
        random.shuffle(run.tests)
    else:
        run.tests.sort(key = lambda t: t.getFullName())

    # Finally limit the number of tests, if desired.
    if opts.maxTests is not None:
        run.tests = run.tests[:opts.maxTests]

    # Don't create more threads than tests.
    opts.numThreads = min(len(run.tests), opts.numThreads)

    extra = ''
    if len(run.tests) != numTotalTests:
        extra = ' of %d' % numTotalTests
    header = '-- Testing: %d%s tests, %d threads --'%(len(run.tests), extra,
                                                      opts.numThreads)

    progressBar = None
    if not opts.quiet:
        if opts.succinct and opts.useProgressBar:
            try:
                tc = lit.ProgressBar.TerminalController()
                progressBar = lit.ProgressBar.ProgressBar(tc, header)
            except ValueError:
                print(header)
                progressBar = lit.ProgressBar.SimpleProgressBar('Testing: ')
        else:
            print(header)

    startTime = time.time()
    display = TestingProgressDisplay(opts, len(run.tests), progressBar)
    provider = TestProvider(run.tests, opts.maxTime)

    try:
      import win32api
    except ImportError:
      pass
    else:
      def console_ctrl_handler(type):
        provider.cancel()
        return True
      win32api.SetConsoleCtrlHandler(console_ctrl_handler, True)

    runTests(opts.numThreads, litConfig, provider, display)
    display.finish()

    if not opts.quiet:
        print('Testing Time: %.2fs'%(time.time() - startTime))

    # Update results for any tests which weren't run.
    for test in run.tests:
        if test.result is None:
            test.setResult(lit.Test.Result(lit.Test.UNRESOLVED, '', 0.0))

    # List test results organized by kind.
    hasFailures = False
    byCode = {}
    for test in run.tests:
        if test.result.code not in byCode:
            byCode[test.result.code] = []
        byCode[test.result.code].append(test)
        if test.result.code.isFailure:
            hasFailures = True

    # Print each test in any of the failing groups.
    for title,code in (('Unexpected Passing Tests', lit.Test.XPASS),
                       ('Failing Tests', lit.Test.FAIL),
                       ('Unresolved Tests', lit.Test.UNRESOLVED)):
        elts = byCode.get(code)
        if not elts:
            continue
        print('*'*20)
        print('%s (%d):' % (title, len(elts)))
        for test in elts:
            print('    %s' % test.getFullName())
        sys.stdout.write('\n')

    if opts.timeTests and run.tests:
        # Order by time.
        test_times = [(test.getFullName(), test.result.elapsed)
                      for test in run.tests]
        lit.util.printHistogram(test_times, title='Tests')

    for name,code in (('Expected Passes    ', lit.Test.PASS),
                      ('Expected Failures  ', lit.Test.XFAIL),
                      ('Unsupported Tests  ', lit.Test.UNSUPPORTED),
                      ('Unresolved Tests   ', lit.Test.UNRESOLVED),
                      ('Unexpected Passes  ', lit.Test.XPASS),
                      ('Unexpected Failures', lit.Test.FAIL),):
        if opts.quiet and not code.isFailure:
            continue
        N = len(byCode.get(code,[]))
        if N:
            print('  %s: %d' % (name,N))

    # If we encountered any additional errors, exit abnormally.
    if litConfig.numErrors:
        sys.stderr.write('\n%d error(s), exiting.\n' % litConfig.numErrors)
        sys.exit(2)

    # Warn about warnings.
    if litConfig.numWarnings:
        sys.stderr.write('\n%d warning(s) in tests.\n' % litConfig.numWarnings)

    if hasFailures:
        sys.exit(1)
    sys.exit(0)

if __name__=='__main__':
    main()
