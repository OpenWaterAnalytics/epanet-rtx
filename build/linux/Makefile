# epanet-rtx makefile

# *** Filenames
RTXNAME = epanetrtx
RTXLIBNAME = lib$(RTXNAME).so

# ** Paths
OBJPATH = ./obj
EPANETSRCPATH = ../../src/epanet/src
EPANETINCPATH = ../../src/epanet/include
RTXSRCPATH = ../../src
DEMOSRCPATH = ../../examples/conceptual
VALIDATORSRCPATH = ../../examples/validator
INSTALLPATH = ./bin
INCLUDEPATH = $(EPANETINCPATH) $(RTXSRCPATH) $(EPANETSRCPATH)
INCLUDEARGS = -I$(EPANETINCPATH) -I$(RTXSRCPATH) -I$(EPANETSRCPATH)
VPATH = $(EPANETSRCPATH):$(EPANETINCPATH):$(RTXSRCPATH):$(VALIDATORSRCPATH):$(DEMOSRCPATH)

# *** compiler options
CPP_COMPILER = clang++
CPP_FLAGS = -g -Wall $(INCLUDEARGS)
C_COMPILER = clang
C_FLAGS = -w -fPIC $(INCLUDEARGS)
LDFLAGS = -L. -L/usr/local/lib -v

# *** Files
RTX_HEADERS = AggregatorTimeSeries.h Clock.h ConfigFactory.h CurveFunction.h DbPointRecord.h Element.h EpanetModel.h EpanetSyntheticModel.cpp EpanetSyntheticModel.h FirstDerivative.h IrregularClock.h Junction.h Link.h Model.h ModularTimeSeries.h MovingAverage.h MysqlPointRecord.h Node.h Pipe.h Point.h PointRecord.h Pump.h Resampler.h Reservoir.h Tank.h TimeSeries.h Units.h Valve.h Zone.h rtxExceptions.h rtxMacros.h

RTX_SRC = AggregatorTimeSeries.cpp Clock.cpp ConfigFactory.cpp CurveFunction.cpp Element.cpp EpanetModel.cpp EpanetSyntheticModel.cpp FirstDerivative.cpp IrregularClock.cpp Junction.cpp Link.cpp Model.cpp ModularTimeSeries.cpp MovingAverage.cpp MysqlPointRecord.cpp Node.cpp Pipe.cpp Point.cpp PointRecord.cpp Pump.cpp Resampler.cpp Reservoir.cpp Tank.cpp TimeSeries.cpp Units.cpp Valve.cpp Zone.cpp

RTX_OBJS = AggregatorTimeSeries.o Clock.o ConfigFactory.o CurveFunction.o Element.o EpanetModel.o EpanetSyntheticModel.o FirstDerivative.o IrregularClock.o Junction.o Link.o Model.o ModularTimeSeries.o MovingAverage.o MysqlPointRecord.o Node.o Pipe.o Point.o PointRecord.o Pump.o Resampler.o Reservoir.o Tank.o TimeSeries.o Units.o Valve.o Zone.o

EPANET_SRC = epanet.c hash.c hydraul.c inpfile.c input1.c input2.c input3.c mempool.c output.c quality.c report.c rules.c smatrix.c

EPANET_OBJS = epanet.o hash.o hydraul.o inpfile.o input1.o input2.o input3.o mempool.o output.o quality.o report.o rules.o smatrix.o


# *** Targets
.PHONY: all
all: getobj $(RTXLIBNAME) examples putobj

.PHONY: examples
examples: $(RTXLIBNAME) rtx-demo rtx-validator

.PHONY: clean
clean:
	-@rm -rf *.o $(OBJPATH) $(RTXLIBNAME) rtx-demo rtx-validator 2> /dev/null

putobj:
	-@mkdir $(OBJPATH) 2> /dev/null
	-@mv -f *.o $(OBJPATH) 2> /dev/null

getobj:
	-@mv -f $(OBJPATH)/*.o ./ 2> /dev/null
	-@rm -rf $(OBJPATH) 2> /dev/null

rtx-demo: timeseries_demo.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o $@ $^ $(LDFLAGS) -l$(RTXNAME) -lboost_system

timeseries_demo.o: timeseries_demo.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -c $^

rtx-validator: validator.o
	$(CPP_COMPILER) $(CPP_FLAGS) -o $@ $^ $(LDFLAGS) -l$(RTXNAME) -lboost_system

validator.o: validator.cpp
	$(CPP_COMPILER) $(CPP_FLAGS) -c $^

$(RTXLIBNAME): $(EPANET_OBJS) $(RTX_OBJS)
	$(CPP_COMPILER) $(CPP_FLAGS) -shared -o $@ $^ $(LDFLAGS) -lconfig++ -lboost_system -lboost_filesystem -lboost_date_time -lmysqlcppconn -liodbc

$(RTX_OBJS): $(RTX_SRC)
	$(CPP_COMPILER) $(CPP_FLAGS) -c $^

$(EPANET_OBJS) : $(EPANET_SRC)
	$(C_COMPILER) $(C_FLAGS) -c $^

