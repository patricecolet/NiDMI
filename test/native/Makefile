CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../stubs -DNIDMI_COMPONENT_DEFS_LIGHT
SRCDIR   = ../../src

TESTS = test_analog_filter test_hysteresis test_axis_utils test_json_parser test_component_json

.PHONY: all clean run

all: $(TESTS)

run: all
	@fail=0; \
	for t in $(TESTS); do \
		echo ""; \
		./$$t || fail=1; \
	done; \
	echo ""; \
	if [ $$fail -eq 0 ]; then \
		echo "All test suites passed."; \
	else \
		echo "Some tests FAILED."; exit 1; \
	fi

test_analog_filter: test_analog_filter.cpp $(SRCDIR)/utils/AnalogFilter.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_hysteresis: test_hysteresis.cpp $(SRCDIR)/utils/Hysteresis.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_axis_utils: test_axis_utils.cpp $(SRCDIR)/utils/AxisUtils.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test_json_parser: test_json_parser.cpp $(SRCDIR)/utils/JSONParser.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(SRCDIR)/utils/JSONParser.cpp

test_component_json: test_component_json.cpp ../stubs/MidiMessageType_stub.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< ../stubs/MidiMessageType_stub.cpp

clean:
	rm -f $(TESTS)
