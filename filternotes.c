/*matlab : 
freqz([coeffs])
shows how different frequencies are affected by filter

fir1(order, cuttoff freq)
input order and cuttoff and returns coeffs for that filter (low pass)

*/

//MAF - moving average filter

int rawData[order];
for(int i = 0; i < order; i++) rawData[i] = 0;

while(1) {
	rawData[i] = readsensor();
	filteredData[j] = (sum rawData) / filter_order;
	j++;
	i = (i + 1) % order;
}

//FIR - finite impulse response
int order = 8;
int rawData;
for(int i = 0; i < 100; i++) rawData[i] = 0;
int intermed[order];
int sum;
int filteredData[100];

while(1) {
	sum = 0;
	rawData = readsensor();
	for(int i = order-1; i >= 0; i++) {
		intermed[i] = intermed[i-1];
	}
	intermed[0] = rawData[i];
	for(int i = 0; i < order; i++) {
		sum += intermed[i] * coeffs[i];
	}
	filteredData[j] = sum;
	j++
}

//IIR - infinite impulse response

float A = 0.8;
float B = 0.2;

while(1) {
	rawData = readsensor();
	filteredData[j] = A*filteredData[j-1] + B*rawData;
	//where A + B = 1
	j++;
}