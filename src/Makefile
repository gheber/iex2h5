#
#   ALL RIGHTS RESERVED.
#   _________________________________________________________________________________
#   NOTICE: All information contained  herein is, and remains the property  of  Varga
#   Consulting and  its suppliers, if  any. The intellectual and  technical  concepts
#   contained herein are proprietary to Varga Consulting and its suppliers and may be
#   covered  by  Canadian and  Foreign Patents, patents in process, and are protected
#   by  trade secret or copyright law. Dissemination of this information or reproduc-
#   tion  of  this  material is strictly forbidden unless prior written permission is
#   obtained from Varga Consulting.
#
#   Copyright © <2017-2025> Varga Consulting, Toronto, On     info@vargaconsulting.ca
#   _________________________________________________________________________________

app = iex2h5
LIBS = -lpcap -pthread -ldate-tz -lcurl \
	-lm -lz  -ldl -lpthread 			\
	`pkg-config --libs armadillo`		\
	`pkg-config --libs hdf5`

CXXFLAGS =  -I ../include -I/usr/local/include \
	-Wno-deprecated-declarations \
	`pkg-config --cflags hdf5` -std=c++23  	\
	-DARMA_DONT_USE_WRAPPER 				\
	-DHOST_NAME_MAX=254 					\
	-DVALUE_PRECISION=6 -DSTAT_PRECISION=4  \
	-DDEBUG -DIEX_MAX_SYMBOLS=20000 -g -O3 -no-pie

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

iex2h5: $(app).o
	$(CXX) $^ $(LIBS) -Wl,-rpath=$(pwd)  -o $@ 	

all: $(app) 

install:
	cp $(app) /usr/local/bin/

clean:
	@$(RM) *.o *.prof  $(app)

# iex-download --tops --from 2017-01-01 --to 2017-01-08
tops_files :=$(sort $(wildcard ../data/*))
test-tops: $(app) $(HOME)/iex.h5
	$(foreach dp, $(tops_files), unpigz -c $(dp) | ./$(app) --time-interval 20 --command rts -o $(HOME)/iex.h5; )
	./$(app) -o $(HOME)/iex.h5 --command index

$(HOME)/iex.h5: $(app)
	unpigz -c ../data/TOPS-2017-01-06.pcap.gz | ./$(app) --command init -o $(HOME)/iex.h5

stats: $(app)
	unpigz -c ../data/TOPS-2017-01-06.pcap.gz | ./$(app) --command stats -o $(HOME)/iex.h5

init: $(app)
	unpigz -c ../data/TOPS-2017-01-06.pcap.gz | ./$(app) --command init -o $(HOME)/iex.h5

irts: $(app)
	unpigz -c ../data/TOPS-2017-01-06.pcap.gz | ./$(app) --command irts -o $(HOME)/iex.h5

rts: $(app)
	unpigz -c ../data/TOPS-2017-01-06.pcap.gz | ./$(app) --command rts -o $(HOME)/iex.h5

index: $(app)
	./$(app) --command index -o $(HOME)/iex.h5


.PHONY: 
