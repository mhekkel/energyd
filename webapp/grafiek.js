import "core-js/stable";
import "regenerator-runtime/runtime";
import 'bootstrap';
import * as d3 from 'd3';

import './style.scss';

function nest(data) {

	// const t = data.map(d => {
	// 	return {
	// 		key: d.date.getFullYear(),
	// 		value: d
	// 	};
	// });



	// const t = data.reduce((obj, { date, year, xdate, verbruik }) => {
	// 	if (!("key" in obj)) {
	// 		obj = {
	// 			key: date.getFullYear(),
	// 			values: [
	// 				{
	// 					xdate: xdate,
	// 					verbruik: verbruik
	// 				}
	// 			]
	// 		}
	// 	}
	// 	else {
			
	// 	}
	// 	return obj;
	// }, {});

	// console.log(t);


	// // Now transform it into the desired format
	// const result = Object.entries(nameYearsCount)
	// 	.map(([name, yearsCount]) => {
	// 		const values = Object.entries(yearsCount)
	// 			.map(([year, count]) => ({
	// 				key: year,
	// 				value: count
	// 			}));
	// 		return {
	// 			key: name,
	// 			values
	// 		};
	// 	});

}

class grafiek {
	constructor() {

		this.selector = document.getElementById('grafiek-id');
		this.selector.addEventListener('change', ev => {
			if (ev)
				ev.preventDefault();
			this.laadGrafiek();
		});

		for (let btn of document.getElementsByClassName("aggr-btn")) {
			if (btn.checked)
				this.aggrType = btn.dataset.aggr;
			btn.onchange = (e) => this.selectAggrType(e.target.dataset.aggr);
		}

		this.svg = d3.select("svg");

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
			.text("Datum");

		this.svg.append("text")
			.attr("class", "y axis-label")
			.attr("text-anchor", "end")
			.attr("x", -this.margin.top)
			.attr("y", 6)
			.attr("dy", ".75em")
			.attr("transform", "rotate(-90)")
			.text("Verbruik per dag");

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

		const zoom = d3.zoom()
			.scaleExtent([1, 40])
			.translateExtent([[0, 0], [this.width + 90, this.height + 90]])
			.on("zoom", () => this.zoomed());

		this.svg.call(zoom);
	}

	laadGrafiek() {
		const selected = this.selector.selectedOptions;
		if (selected.length !== 1)
			throw ("ongeldige keuze");

		const keuze = selected.item(0).value;
		const grafiekNaam = selected.item(0).textContent;

		let grafiekTitel = document.querySelector(".grafiek-titel");
		if (grafiekTitel.classList.contains("grafiek-status-loading"))  // avoid multiple runs
			return;

		grafiekTitel.classList.add("grafiek-status-loading");
		grafiekTitel.classList.remove("grafiek-status-loaded")
		grafiekTitel.classList.remove("grafiek-status-failed");

		Array.from(document.getElementsByClassName("grafiek-naam"))
			.forEach(span => span.textContent = grafiekNaam);

		fetch(`ajax/data/${keuze}/${this.aggrType}`, {
			credentials: "include",
			headers: {
				'Accept': 'application/json'
			}
		}).then(async response => {
			if (response.ok)
				return response.json();

			const error = await response.json();
			console.log(error);
			throw error.message;
		}).then(data => {
			this.processData(data);
			grafiekTitel.classList.remove("grafiek-status-loading");
			grafiekTitel.classList.add("grafiek-status-loaded");
		}).catch(err => {
			grafiekTitel.classList.remove("grafiek-status-loading");
			grafiekTitel.classList.add("grafiek-status-failed");
			console.log(err);
		});
	}

	processData(data) {
		const curve = d3.curveBasis;

		const x = (d) => new Date(d.d);
		const y = (d) => d.v;
		const y_a = (d) => d.a;
		const z = () => 1;
		const zsdp = (d) => d.a + d.sd;
		const zsdm = (d) => d.a - d.sd;

		const X = d3.map(data, x);
		const Y = d3.map(data, y);
		const Y_a = d3.map(data, y_a);
		const Z = d3.map(data, z); //['v', 'v-sd', 'v+sd', 'ma'];

		const Zsdp = d3.map(data, zsdp); //['v', 'v-sd', 'v+sd', 'ma'];
		const Zsdm = d3.map(data, zsdm); //['v', 'v-sd', 'v+sd', 'ma'];

		const defined_v = (d) => d.v != 0;
		const defined_a = (d) => d.a != 0;

		const Dv = d3.map(data, defined_v);
		const Da = d3.map(data, defined_a);

		const xDomain = d3.extent(X);
		const yDomain = d3.extent(Y);
		const zDomain = new d3.InternSet(Z);

		const I = d3.range(X.length);//.filter(i => zDomain.has(Z[i]));

		const xType = d3.scaleUtc;
		const yType = d3.scaleLinear;

		const xRange = [1, this.width - 2];
		const yRange = [this.height - 2, 1];

		const xScale = xType(xDomain, xRange).interpolate(d3.interpolateRound);
		const yScale = yType(yDomain, yRange);

		const colors = d3.scaleOrdinal([1], d3.schemeCategory10);

		const xFormat = null;
		const yFormat = "";

		const xAxis = d3.axisBottom(xScale).ticks(this.width / 80, xFormat).tickSizeOuter(0);
		const yAxis = d3.axisLeft(yScale).ticks(null, yFormat);
	  
		const formatDate = xScale.tickFormat(null, "%j");

		const line_v = d3.line()
			.defined(i => Dv[i])
			.curve(curve)
			.x(i => xScale(X[i]))
			.y(i => yScale(Y[i]));
		
		const line_a = d3.line()
			.defined(i => Da[i])
			.curve(curve)
			.x(i => xScale(X[i]))
			.y(i => yScale(Y_a[i]));
		
		const area = d3.area()
			.defined(i => Da[i])
			.curve(curve)
			.x(i => xScale(X[i]))
			.y0(i => yScale(Zsdm[i]))
			.y1(i => yScale(Zsdp[i]));
  
		this.gX.call(xAxis);
		this.gY.call(yAxis);

		this.plotData.selectAll(".sd")
			.remove();

		this.plotData.selectAll(".sd")
			.data(d3.group(I, i => Z[i]))
			.enter()
			.append("path")
			.attr("fill", "#ddd")
			.attr("d", ([, i]) => area(i));


		this.plotData.selectAll(".line-a")
			.remove();

		this.plotData.selectAll(".line-a")
			.data(d3.group(I, i => Z[i]))
			.enter()
			.append("path")
			.attr("fill", "none")
			.attr("stroke-width", 1.5)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", colors(1))
			.attr("d", ([, i]) => line_a(i));

		this.plotData.selectAll(".line")
			.remove();

		this.plotData.selectAll(".line")
			.data(d3.group(I, i => Z[i]))
			.enter()
			.append("path")
			.attr("fill", "none")
			.attr("stroke-width", 1.5)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", colors(0))
			.attr("d", ([, i]) => line_v(i));

		

		// this.plotData.selectAll(".line")
		// 	.data(dataPunten2)
		// 	.enter()
		// 	.append("path")
		// 	.attr("class", "line")
		// 	.attr("fill", "none")
		// 	.attr("opacity", d => 1 / (lastYear - d.key + 1))
		// 	// .attr("opacity", d => 1.5 / (lastYear - d.key + 1.5))
		// 	.attr("stroke", d => colors(d.key))
		// 	.attr("stroke-width", 1.5)
		// 	.attr("d", d =>
		// 		d3.line()
		// 			.x(d => x(d.xdate))
		// 			.y(d => y(d.verbruik))(d.values)
		// 	);

		// // voortschrijdend gemiddelde

		// const dataPunten3 = [];
		// for (let d in data.vsgem) {
		// 	const date = new Date(d);

		// 	if (!years.has(date.getFullYear()))
		// 		console.log("how is this possible, year not known yet");

		// 	dataPunten3.push({
		// 		date: date,
		// 		year: date.getFullYear(),
		// 		xdate: d3.timeFormat("%j")(date),
		// 		verbruik: data.vsgem[d]
		// 	});
		// }

		// const dataPunten4 = d3.nest()
		// 	.key(d => d.date.getFullYear())
		// 	.entries(dataPunten3);

		// this.plotData.selectAll(".ma-line")
		// 	.remove();

		// this.plotData.selectAll(".ma-line")
		// 	.data(dataPunten4)
		// 	.enter()
		// 	.append("path")
		// 	.attr("class", "ma-line")
		// 	.attr("fill", "none")
		// 	.attr("stroke", "gray")
		// 	.attr("stroke-width", 2)
		// 	.attr("d", d =>
		// 		d3.line()
		// 			.x(d => x(d.xdate))
		// 			.y(d => y(d.verbruik))(d.values)
		// 	);

	}

	zoomed() {

	}

	selectAggrType(aggr) {
		this.aggrType = aggr;
		this.laadGrafiek()
	}
}


window.addEventListener("load", () => {
	const g = new grafiek();
	g.laadGrafiek();
});


// // Copyright 2021 Observable, Inc.
// // Released under the ISC license.
// // https://observablehq.com/@d3/index-chart
// function IndexChart(data, {
// 	x = ([x]) => x, // given d in data, returns the (temporal) x-value
// 	y = ([, y]) => y, // given d in data, returns the (quantitative) y-value
// 	z = () => 1, // given d in data, returns the (categorical) z-value for series
// 	defined, // for gaps in data
// 	curve = d3.curveLinear, // how to interpolate between points
// 	marginTop = 20, // top margin, in pixels
// 	marginRight = 40, // right margin, in pixels
// 	marginBottom = 30, // bottom margin, in pixels
// 	marginLeft = 40, // left margin, in pixels
// 	width = 640, // outer width, in pixels
// 	height = 400, // outer height, in pixels
// 	xType = d3.scaleUtc, // the x-scale type
// 	xDomain, // [xmin, xmax]
// 	xRange = [marginLeft, width - marginRight], // [left, right]
// 	xFormat, // a format specifier string for the x-axis
// 	yType = d3.scaleLog, // the y-scale type
// 	yDomain, // [ymin, ymax]
// 	yRange = [height - marginBottom, marginTop], // [bottom, top]
// 	yFormat = "", // a format specifier string for the y-axis
// 	yLabel, // a label for the y-axis
// 	zDomain, // array of z-values
// 	formatDate = "%b %-d, %Y", // format specifier string for dates (in the title)
// 	colors = d3.schemeTableau10, // array of categorical colors
//   } = {}) {
// 	// Compute values.
// 	const X = d3.map(data, x);
// 	const Y = d3.map(data, y);
// 	const Z = d3.map(data, z);
// 	if (defined === undefined) defined = (d, i) => !isNaN(X[i]) && !isNaN(Y[i]);
// 	const D = d3.map(data, defined);
  
// 	// Compute default x- and z-domains, and unique the z-domain.
// 	if (xDomain === undefined) xDomain = d3.extent(X);
// 	if (zDomain === undefined) zDomain = Z;
// 	zDomain = new d3.InternSet(zDomain);
  
// 	// Omit any data not present in the z-domain.
// 	const I = d3.range(X.length).filter(i => zDomain.has(Z[i]));
// 	const Xs = d3.sort(I.filter(i => D[i]).map(i => X[i])); // for bisection later
  
// 	// Compute default y-domain.
// 	if (yDomain === undefined) {
// 	  const r = I => d3.max(I, i => Y[i]) / d3.min(I, i => Y[i]);
// 	  const k = d3.max(d3.rollup(I, r, i => Z[i]).values());
// 	  yDomain = [1 / k, k];
// 	}
  
// 	// Construct scales and axes.
// 	const xScale = xType(xDomain, xRange).interpolate(d3.interpolateRound);
// 	const yScale = yType(yDomain, yRange);
// 	const color = d3.scaleOrdinal(zDomain, colors);
// 	const xAxis = d3.axisBottom(xScale).ticks(width / 80, xFormat).tickSizeOuter(0);
// 	const yAxis = d3.axisLeft(yScale).ticks(null, yFormat);
  
// 	// Construct formats.
// 	formatDate = xScale.tickFormat(null, formatDate);
  
// 	// Construct a line generator.
// 	const line = d3.line()
// 		.defined(i => D[i])
// 		.curve(curve)
// 		.x(i => xScale(X[i]))
// 		.y((i, _, I) => yScale(Y[i] / Y[I[0]]));
  
// 	const svg = d3.create("svg")
// 		.attr("width", width)
// 		.attr("height", height)
// 		.attr("viewBox", [0, 0, width, height])
// 		.attr("style", "max-width: 100%; height: auto; height: intrinsic;")
// 		.attr("font-family", "sans-serif")
// 		.attr("font-size", 10)
// 		.on("touchstart", event => event.preventDefault())
// 		.on("pointermove", pointermoved);
  
// 	svg.append("g")
// 		.attr("transform", `translate(0,${height - marginBottom})`)
// 		.call(xAxis)
// 		.call(g => g.select(".domain").remove());
  
// 	svg.append("g")
// 		.attr("transform", `translate(${marginLeft},0)`)
// 		.call(yAxis)
// 		.call(g => g.select(".domain").remove())
// 		.call(g => g.selectAll(".tick line").clone()
// 			.attr("stroke-opacity", d => d === 1 ? null : 0.2)
// 			.attr("x2", width - marginLeft - marginRight))
// 		.call(g => g.append("text")
// 			.attr("x", -marginLeft)
// 			.attr("y", 10)
// 			.attr("fill", "currentColor")
// 			.attr("text-anchor", "start")
// 			.text(yLabel));
  
// 	const rule = svg.append("g");
  
// 	rule.append("line")
// 		.attr("y1", marginTop)
// 		.attr("y2", height - marginBottom - 15)
// 		.attr("stroke", "currentColor");
  
// 	const ruleLabel = rule.append("text")
// 		.attr("y", height - marginBottom - 15)
// 		.attr("fill", "currentColor")
// 		.attr("text-anchor", "middle")
// 		.attr("dy", "1em");
  
// 	const serie = svg.append("g")
// 	  .selectAll("g")
// 	  .data(d3.group(I, i => Z[i]))
// 	  .join("g");
  
// 	serie.append("path")
// 		.attr("fill", "none")
// 		.attr("stroke-width", 1.5)
// 		.attr("stroke-linejoin", "round")
// 		.attr("stroke-linecap", "round")
// 		.attr("stroke", ([z]) => color(z))
// 		.attr("d", ([, I]) => line(I));
  
// 	serie.append("text")
// 		.attr("font-weight", "bold")
// 		.attr("fill", "none")
// 		.attr("stroke", "white")
// 		.attr("stroke-width", 3)
// 		.attr("stroke-linejoin", "round")
// 		.attr("x", ([, I]) => xScale(X[I[I.length - 1]]))
// 		.attr("y", ([, I]) => yScale(Y[I[I.length - 1]] / Y[I[0]]))
// 		.attr("dx", 3)
// 		.attr("dy", "0.35em")
// 		.text(([z]) => z)
// 	  .clone(true)
// 		.attr("fill", ([z]) => color(z))
// 		.attr("stroke", null);
  
// 	function update(date) {
// 	  date = Xs[d3.bisectCenter(Xs, date)];
// 	  rule.attr("transform", `translate(${xScale(date)},0)`);
// 	  ruleLabel.text(formatDate(date));
// 	  serie.attr("transform", ([, I]) => {
// 		const i = I[d3.bisector(i => X[i]).center(I, date)];
// 		return `translate(0,${yScale(1) - yScale(Y[i] / Y[I[0]])})`;
// 	  });
// 	  svg.property("value", date).dispatch("input", {bubbles: true}); // for viewof
// 	}
  
// 	function pointermoved(event) {
// 	  update(xScale.invert(d3.pointer(event)[0]));
// 	}
  
// 	update(xDomain[0]);
  
// 	return Object.assign(svg.node(), {scales: {color}, update});
//   }