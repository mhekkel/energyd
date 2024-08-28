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

		this.gY1 = this.g.append("g")
			.attr("class", "axis axis--y");

		this.gY2 = this.g.append("g")
			.attr("class", "axis axis--y")
			.attr("transform", `translate(${this.width}, 0)`);

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

		if (this.width < 100)
			return;
		
		const resolutie = 15;

		const data = await fetch(`ajax/grafiek/${datum.toISOString().substring(0, 10)}?resolutie=${resolutie}`)
			.then(async r => {
				if (r.ok)
					return r.json();

				const error = await r.json();
				throw error.message;
			});

		console.log(data);

		let minE = 0, maxE = 0;

		data.forEach(d => {
			d.tijd = new Date(d.tijd);
			minE = Math.min(minE, d.zon, d.batterij, d.verbruik, d.levering, d.laad_niveau);
			maxE = Math.max(maxE, d.zon, d.batterij, d.verbruik, d.levering, d.laad_niveau);
		});

		const x = d3.scaleTime([start, stop], [1, this.width - 2]);
		const y_soc = d3.scaleLinear([0, 1], [this.height - 2, 1]);
		const y = d3.scaleLinear([minE, maxE], [this.height - 2, 1]);
		const colors = d3.scaleOrdinal(d3.schemeTableau10);

		const xFormat = d3.timeFormat("%H");

		const xAxis = d3.axisBottom(x).tickFormat(xFormat).tickSizeOuter(0);//.ticks(d3.timeHour.every(1), xFormat);
		const y_Axis = d3.axisLeft(y);
		const y_soc_Axis = d3.axisRight(y_soc)/* .tickSizeInner(this.width) */;

		const line_soc = d3.line()
			.curve(d3.curveBasis)
			.x(d => x(d.tijd))
			.y(d => y_soc(d.laad_niveau));

		this.gX.call(xAxis);
		this.gY1.call(y_Axis);
		this.gY2.call(y_soc_Axis);

		this.plotData.append("path")
			.attr("class", "soc")
			.attr("fill", "none")
			.attr("stroke", colors(0))
			.attr("stroke-width", 1.5)
			.attr("d", line_soc(data));

		const barWidth = (this.width - 2) / ((24*60) / resolutie);

		const barData = [
			[0, d3.schemeTableau10[2], 'verbruik', -1],
			[1, d3.schemeTableau10[4], 'levering', 1],
			[2, d3.schemeTableau10[1], 'zon', 1],
			[3, d3.schemeTableau10[3], 'batterij', 1]
		];

		barData.forEach(([dx, color, bar, f]) => {
			this.plotData.append("g")
				.attr("fill", color)
				.selectAll(bar)
				.data(data)
				.join("rect")
				.attr("class", bar)
				.attr("x", d => x(d.tijd) + dx * (barWidth / 5))
				.attr("y", d => f * d[bar] > 0 ? y(f * d[bar]) : y(0))
				.attr("width", barWidth / 5)
				.attr("height", d => Math.abs(y(0) - y(f * d[bar])));
		});


		// this.plotData.append("g")
		// 	.attr("fill", colors(2))
		// 	.selectAll(".sun")
		// 	.data(data)
		// 	.join("rect")
		// 	.attr("class", "sun")
		// 	.attr("x", d => x(d.tijd) + 3)
		// 	.attr("y", d => d.zon > 0 ? y(d.zon) : y(0))
		// 	.attr("width", 2)
		// 	.attr("height", d => Math.abs(y(0) - y(d.zon)));

		// this.plotData.append("g")
		// 	.attr("fill", colors(3))
		// 	.selectAll(".use")
		// 	.data(data)
		// 	.join("rect")
		// 	.attr("class", "use")
		// 	.attr("x", d => x(d.tijd) + 6)
		// 	.attr("y", d => y(0))
		// 	.attr("width", 2)
		// 	.attr("height", d => Math.abs(y(0) - y(d.verbruik)));

		// this.plotData.append("g")
		// 	.attr("fill", colors(4))
		// 	.selectAll(".provide")
		// 	.data(data)
		// 	.join("rect")
		// 	.attr("class", "provide")
		// 	.attr("x", d => x(d.tijd) + 9)
		// 	.attr("y", d => d.levering > 0 ? y(d.levering) : y(0))
		// 	.attr("width", 2)
		// 	.attr("height", d => Math.abs(y(0) - y(d.levering)));


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