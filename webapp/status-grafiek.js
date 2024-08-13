import * as d3 from 'd3';

let grafiek;

class Grafiek {
	constructor() {

		this.svg = d3.select("svg");
		this.svg.on("touchstart", event => event.preventDefault());

		this.maakGrafiek();
	}

	maakGrafiek() {
		const plotContainer = this.svg.node();
		let bBoxWidth = plotContainer.clientWidth;
		let bBoxHeight = plotContainer.clientHeight;

		if ((bBoxWidth * 9 / 16) > bBoxHeight)
			bBoxWidth = 16 * bBoxHeight / 9;
		else
			bBoxHeight = 9 * bBoxWidth / 16;

		this.margin = { top: 30, right: 50, bottom: 30, left: 50 };

		this.width = bBoxWidth - this.margin.left - this.margin.right;
		this.height = bBoxHeight - this.margin.top - this.margin.bottom;

		this.defs = this.svg.append('defs');

		this.defs
			.append("svg:clipPath")
			.attr("id", "clip")
			.append("svg:rect")
			.attr("x", 0)
			.attr("y", 0)
			.attr("width", this.width)
			.attr("height", this.height);

		this.svg.append("text")
			.attr("class", "x axis-label")
			.attr("text-anchor", "end")
			.attr("x", this.width + this.margin.left)
			.attr("y", this.height + this.margin.top + this.margin.bottom)
			.text("Tijd");

		// this.svg.append("text")
		// 	.attr("class", "y axis-label")
		// 	.attr("text-anchor", "end")
		// 	.attr("x", -this.margin.top)
		// 	.attr("y", 6)
		// 	.attr("dy", ".75em")
		// 	.attr("transform", "rotate(-90)")
		// 	.text("Verbruik per dag");

		this.g = this.svg.append("g")
			.attr("transform", `translate(${this.margin.left},${this.margin.top})`);

		this.gX = this.g.append("g")
			.attr("class", "axis axis--x")
			.attr("transform", `translate(0,${this.height})`);

		this.gY = this.g.append("g")
			.attr("class", "axis axis--y");

		this.plot = this.g.append("g")
			.attr("class", "plot")
			.attr("width", this.width)
			.attr("height", this.height)
			.attr("clip-path", "url(#clip)");

		this.plotData = this.plot.append('g')
			.attr("width", this.width)
			.attr("height", this.height);

		// const zoom = d3.zoom()
		// 	.scaleExtent([1, 40])
		// 	.translateExtent([[0, 0], [this.width + 90, this.height + 90]])
		// 	.on("zoom", () => this.zoomed());

		// const zoom = d3.zoom()
		// 	.scaleExtent([1, 1]);

		// this.svg.call(zoom)
		// 	.on("wheel.zoom", null)
		// 	.on("touchstart", null)
		// 	.on("touchmove", null);
	}

	herstelGrafiek() {
		this.svg.selectAll("#clip").remove();
		this.svg.selectAll("text").remove();
		this.svg.selectAll("g").remove();

		this.maakGrafiek();
		this.laadGrafiek();
	}

	async laadGrafiek() {

		const datum = new Date();
		const start = d3.timeDay.floor(datum);
		const stop = d3.timeDay.ceil(datum);
		const range = d3.timeDay.range(start, stop);


		const data = await fetch(`ajax/grafiek/${datum.toISOString().substring(0, 10)}`)
			.then(async r => {
				if (r.ok)
					return r.json();

				const error = await r.json();
				throw error.message;
			});

		
		console.log("data: ", data);

		const x = (d) => d.tijd;
		// const y = (d) => d.v;

		const X = d3.map(data, x);
		// const Y = d3.map(data, y);
		// const Y_a = d3.map(data, (d) => d.a);
		// const Z = d3.map(data, z); //['v', 'v-sd', 'v+sd', 'ma'];
		// const Y_ma = d3.map(data, (d) => d.ma);

		// const Zsdp = d3.map(data, zsdp); //['v', 'v-sd', 'v+sd', 'ma'];
		// const Zsdm = d3.map(data, zsdm); //['v', 'v-sd', 'v+sd', 'ma'];

		// const defined_v = (d) => d.v != 0;
		// const defined_a = (d) => d.a != 0;
		// const defined_ma = (d) => d.ma !== 0;

		// const Dv = d3.map(data, defined_v);
		// const Da = d3.map(data, defined_a);
		// const Dma = d3.map(data, defined_ma);

		const xDomain = [start, stop];
		// const yDomain = [
		// 	d3.min(d3.map(data, (d) => d3.min([d.v, d.a - d.sd]))),
		// 	d3.max(d3.map(data, (d) => d3.max([d.v, d.a + d.sd])))
		// ];

		// const I = d3.range(X.length);//.filter(i => zDomain.has(Z[i]));

		const xType = d3.scaleTime;
		// const yType = d3.scaleLinear;

		const xRange = [1, this.width - 2];
		// const yRange = [this.height - 2, 1];

		const xScale = xType(xDomain, xRange).interpolate(d3.interpolateRound);
		// const yScale = yType(yDomain, yRange);

		// const colors = d3.scaleOrdinal(d3.schemeTableau10);

		const xFormat = d3.timeFormat("%H:%M");

		const xAxis = d3.axisBottom(xScale).tickFormat(xFormat).tickSizeOuter(0);//.ticks(d3.timeHour.every(1), xFormat);
		// const yAxis = d3.axisLeft(yScale).tickSizeInner(-this.width);

		// const line_v = d3.line()
		// 	.defined(i => Dv[i])
		// 	.curve(curve)
		// 	.x(i => xScale(X[i]))
		// 	.y(i => yScale(Y[i]));

		// const line_a = d3.line()
		// 	.defined(i => Da[i])
		// 	.curve(curve)
		// 	.x(i => xScale(X[i]))
		// 	.y(i => yScale(Y_a[i]));

		// const line_ma = d3.line()
		// 	.defined(i => Dma[i])
		// 	.curve(curve)
		// 	.x(i => xScale(X[i]))
		// 	.y(i => yScale(Y_ma[i]));

		// const area = d3.area()
		// 	.defined(i => Da[i])
		// 	.curve(curve)
		// 	.x(i => xScale(X[i]))
		// 	.y0(i => yScale(Zsdm[i]))
		// 	.y1(i => yScale(Zsdp[i]));

		this.gX.call(xAxis);
		// this.gY.call(yAxis);

		// this.plotData.selectAll(".sd")
		// 	.data(d3.group(I, i => Z[i]))
		// 	.join("path")
		// 	.attr("class", "sd")
		// 	.attr("fill", "rgba(200, 236, 255, 0.5)")
		// 	.attr("d", ([, i]) => area(i));

		// this.plotData.selectAll(".line-a")
		// 	.data(d3.group(I, i => Z[i]))
		// 	.join("path")
		// 	.attr("class", "line-a")
		// 	.attr("fill", "none")
		// 	.attr("stroke-width", 1.5)
		// 	.attr("stroke-linejoin", "round")
		// 	.attr("stroke-linecap", "round")
		// 	.attr("stroke", colors(1))
		// 	.attr("d", ([, i]) => line_a(i));

		// this.plotData.selectAll(".line")
		// 	.data(d3.group(I.filter(i => X[i] <= nu), i => Z[i]))
		// 	.join("path")
		// 	.attr("class", "line")
		// 	.attr("fill", "none")
		// 	.attr("stroke-width", 1.5)
		// 	.attr("stroke-linejoin", "round")
		// 	.attr("stroke-linecap", "round")
		// 	.attr("stroke", colors(0))
		// 	.attr("d", ([, i]) => line_v(i));

		// this.plotData.selectAll(".line-l")
		// 	.data(d3.group(I.filter(i => X[i] > nu), i => Z[i]))
		// 	.join("path")
		// 	.attr("class", "line-l")
		// 	.attr("fill", "none")
		// 	.attr("stroke-width", 1.5)
		// 	.attr("stroke-linejoin", "round")
		// 	.attr("stroke-linecap", "round")
		// 	.attr("stroke", colors(2))
		// 	.attr('opacity', 0.6)
		// 	.attr("d", ([, i]) => line_v(i));

		// this.plotData.selectAll(".ma-line")
		// 	.data(d3.group(I, i => Z[i]))
		// 	.join("path")
		// 	.attr("class", "ma-line")
		// 	.attr("fill", "none")
		// 	.attr("stroke-width", 2)
		// 	.attr("stroke-linejoin", "round")
		// 	.attr("stroke-linecap", "round")
		// 	.attr("stroke", "gray")
		// 	.attr("d", ([, i]) => line_ma(i));
	}

	zoomed() {

	}
}

window.addEventListener("load", () => {
	grafiek = new Grafiek();
	grafiek.laadGrafiek();
});

window.addEventListener("resize", () => {
	if (grafiek) {
		grafiek.herstelGrafiek();
	}
})