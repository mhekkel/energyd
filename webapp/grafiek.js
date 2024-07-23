import * as d3 from 'd3';

let grafiek;

class Grafiek {
	constructor() {

		this.selector = document.getElementById('grafiek-id');
		this.selector.addEventListener('change', ev => {
			if (ev)
				ev.preventDefault();
			this.laadGrafiek();
		});

		for (let btn of document.getElementsByClassName("btn-aggr")) {
			if (btn.checked)
				this.aggrType = btn.getAttribute('data-aggr');
			btn.onchange = (e) => this.selectAggrType(e.target.getAttribute('data-aggr'));
		}

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
			this.processData(data/* .filter(d => d.ma !== 0) */);
			grafiekTitel.classList.remove("grafiek-status-loading");
			grafiekTitel.classList.add("grafiek-status-loaded");
		}).catch(err => {
			grafiekTitel.classList.remove("grafiek-status-loading");
			grafiekTitel.classList.add("grafiek-status-failed");
			console.log(err);
		});
	}

	processData(data) {

		const nu_d = new Date(Date.now());
		const nu = new Date(nu_d.getFullYear(), nu_d.getMonth(), nu_d.getDate());
		const morgen = new Date(nu);
		morgen.setDate(nu.getDate() + 1);
		const curve = d3.curveBasis;

		const x = (d) => new Date(d.d);
		const y = (d) => d.v;

		const z = () => 1;
		const zsdp = (d) => d.a + 2 * d.sd;
		const zsdm = (d) => d.a - 2 * d.sd;

		const X = d3.map(data, x);
		const Y = d3.map(data, y);
		const Y_a = d3.map(data, (d) => d.a);
		const Z = d3.map(data, z); //['v', 'v-sd', 'v+sd', 'ma'];
		const Y_ma = d3.map(data, (d) => d.ma);

		const Zsdp = d3.map(data, zsdp); //['v', 'v-sd', 'v+sd', 'ma'];
		const Zsdm = d3.map(data, zsdm); //['v', 'v-sd', 'v+sd', 'ma'];

		const xDomain = d3.extent(X);
		const yDomain = [
			d3.min(d3.map(data, (d) => d3.min([d.v, d.a - d.sd]))),
			d3.max(d3.map(data, (d) => d3.max([d.v, d.a + d.sd])))
		];

		const I = d3.range(X.length);//.filter(i => zDomain.has(Z[i]));

		const xType = d3.scaleUtc;
		const yType = d3.scaleLinear;

		const xRange = [1, this.width - 2];
		const yRange = [this.height - 2, 1];

		const xScale = xType(xDomain, xRange).interpolate(d3.interpolateRound);
		const yScale = yType(yDomain, yRange);

		const colors = d3.scaleOrdinal(d3.schemeTableau10);

		const xFormat = null;

		const xAxis = d3.axisBottom(xScale).ticks(this.width / 80, xFormat).tickSizeOuter(0);
		const yAxis = d3.axisLeft(yScale).tickSizeInner(-this.width);

		const line_v = d3.line()
			.defined(i => data[i].v !== 0)
			.curve(curve)
			.x(i => xScale(X[i]))
			.y(i => yScale(Y[i]));

		const line_a = d3.line()
			.curve(curve)
			.x(i => xScale(X[i]))
			.y(i => yScale(Y_a[i]));

		const line_ma = d3.line()
			.defined(i => data[i].ma !== 0)
			.curve(curve)
			.x(i => xScale(X[i]))
			.y(i => yScale(Y_ma[i]));

		const area = d3.area()
			.curve(curve)
			.x(i => xScale(X[i]))
			.y0(i => yScale(Zsdm[i]))
			.y1(i => yScale(Zsdp[i]));

		this.gX.call(xAxis);
		this.gY.call(yAxis);

		this.plotData.selectAll(".sd")
			.data(d3.group(I, i => Z[i]))
			.join("path")
			.attr("class", "sd")
			.attr("fill", "rgba(200, 236, 255, 0.5)")
			.attr("d", ([, i]) => area(i));

		this.plotData.selectAll(".line-a")
			.data(d3.group(I, i => Z[i]))
			.join("path")
			.attr("class", "line-a")
			.attr("fill", "none")
			.attr("stroke-width", 1.5)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", colors(1))
			.attr("d", ([, i]) => line_a(i));

		this.plotData.selectAll(".line")
			.data(d3.group(I.filter(i => X[i] <= nu), i => Z[i]))
			.join("path")
			.attr("class", "line")
			.attr("fill", "none")
			.attr("stroke-width", 1.5)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", colors(0))
			.attr("d", ([, i]) => line_v(i));

		this.plotData.selectAll(".line-l")
			.data(d3.group(I.filter(i => X[i] >= morgen), i => Z[i]))
			.join("path")
			.attr("class", "line-l")
			.attr("fill", "none")
			.attr("stroke-width", 1.5)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", colors(2))
			.attr('opacity', 0.6)
			.attr("d", ([, i]) => line_v(i));

		this.plotData.selectAll(".ma-line")
			.data(d3.group(I, i => Z[i]))
			.join("path")
			.attr("class", "ma-line")
			.attr("fill", "none")
			.attr("stroke-width", 2)
			.attr("stroke-linejoin", "round")
			.attr("stroke-linecap", "round")
			.attr("stroke", "gray")
			.attr("d", ([, i]) => line_ma(i));
	}

	zoomed() {

	}

	selectAggrType(aggr) {
		this.aggrType = aggr;
		this.laadGrafiek()
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