const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const { CleanWebpackPlugin } = require('clean-webpack-plugin');
const webpack = require('webpack');
const TerserPlugin = require('terser-webpack-plugin');
const UglifyJsPlugin = require('uglifyjs-webpack-plugin');
const path = require('path');

const SCRIPTS = __dirname + "/webapp/";
const SCSS = __dirname + "/scss/";
const DEST = __dirname + "/docroot/";

module.exports = (env) => {

	const PRODUCTION = env != null && env.PRODUCTION;

	const webpackConf = {
		entry: {
			invoer: SCRIPTS + "invoer.js",
			opname: SCRIPTS + "opname.js",
			grafiek: SCRIPTS + "grafiek.js"
		},

		output: {
			path: DEST + "/scripts/",
			filename: "./[name].js"
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
					test: /style\.scss$/,
					use: [
						// PRODUCTION ? MiniCssExtractPlugin.loader : "style-loader",
						"style-loader",
						'css-loader',
						'sass-loader'
					]
				},
				{
					test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
					include: path.resolve(__dirname, './node_modules/bootstrap-icons/font/fonts'),
					type: 'asset/resource',
					generator: {
						filename: 'fonts/[name][ext][query]'
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
			extensions: ['.tsx', '.ts', '.js'],
		},

		plugins: [
			// new CleanWebpackPlugin({
			// 	cleanOnceBeforeBuildPatterns: [
			// 		'scripts/**/*',
			// 		'fonts/**/*'
			// 	]
			// }),
			new MiniCssExtractPlugin({
				filename: './css/[name].css',
				chunkFilename: './css/[id].css'
			})
		],

		optimization: {
			minimizer: [
				new TerserPlugin({ /* additional options here */ }),
				new UglifyJsPlugin({ parallel: 4 })
			]
		}
	};

	if (PRODUCTION) {
		webpackConf.mode = "production";

		webpackConf.plugins.push(
			new CleanWebpackPlugin({
				cleanOnceBeforeBuildPatterns: [
					'scripts/**/*',
					'fonts/**/*'
				]
			}));
	} else {
		webpackConf.mode = "development";
		webpackConf.devtool = 'source-map';
		webpackConf.plugins.push(new webpack.optimize.AggressiveMergingPlugin())
	}

	return webpackConf;
};
