const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const { CleanWebpackPlugin } = require('clean-webpack-plugin');
const webpack = require('webpack');
const TerserPlugin = require('terser-webpack-plugin');
const UglifyJsPlugin = require('uglifyjs-webpack-plugin');
const path = require('path');

const SCRIPTS = __dirname + "/webapp/";
const DEST = __dirname + "/docroot/";

module.exports = (env) => {

	const PRODUCTION = env != null && env.PRODUCTION;

	const webpackConf = {
		entry: {
			invoer: { import: SCRIPTS + "invoer.js", filename: "scripts/[name].js" },
			opname: { import: SCRIPTS + "opname.js", filename: "scripts/[name].js" },
			grafiek: { import: SCRIPTS + "grafiek.js", filename: "scripts/[name].js" },
		},

		output: {
			path: DEST
		},

		devtool: "source-map",

		module: {
			rules: [
				{
					test: /\.js/,
					exclude: /node_modules/,
					use: {
						loader: "babel-loader",
						options: {
							presets: ['@babel/preset-env']
						}
					}
				},

				{
					test: /\.(sa|sc|c)ss$/i,
					use: [
						{
							loader: PRODUCTION ? MiniCssExtractPlugin.loader : "style-loader",
							options: PRODUCTION ? {
								publicPath: "css/",
							} : {}
						},
						{
							loader: "css-loader",
							options: {
								sourceMap: !PRODUCTION
							}
						},
						"postcss-loader",
						{
							loader: "sass-loader",
							options: {
								sourceMap: !PRODUCTION
							}
						},
					],
				},

				{
					test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
					include: path.resolve(__dirname, './node_modules/bootstrap-icons/font/fonts'),
					type: 'asset/resource',
					generator: {
						filename: 'fonts/[name][ext][query]',
						publicPath: '/fonts/'
					}
				},

				{
					test: /\.(png|jpg|gif)$/,
					use: [
						{
							loader: 'file-loader',
							options: {
								outputPath: "css/images",
								publicPath: "images/"
							},
						},
					]
				}
			]
		},

		resolve: {
			extensions: ['.tsx', '.ts', '.js', '.scss'],
		},

		plugins: [],

		optimization: { minimizer: [] }
	};

	if (PRODUCTION) {
		webpackConf.mode = "production";

		webpackConf.plugins.push(
			new MiniCssExtractPlugin({
				filename: "css/[name].css",
				chunkFilename: "css/[id].css",
			}),
			new CleanWebpackPlugin({
				cleanOnceBeforeBuildPatterns: [
					'scripts/**/*',
					'fonts/**/*',
					'css/**'
				]
			}));

		webpackConf.optimization.minimizer.push(
			new TerserPlugin({ /* additional options here */ }),
			new UglifyJsPlugin({ parallel: 4 })
		);
	} else {
		webpackConf.mode = "development";
		webpackConf.devtool = 'source-map';
		webpackConf.plugins.push(new webpack.optimize.AggressiveMergingPlugin())
	}

	return webpackConf;
};
