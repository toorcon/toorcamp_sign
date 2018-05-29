const webpack = require('webpack');
const path = require('path');
const CleanWebpackPlugin = require('clean-webpack-plugin');
const ClosureCompiler = require('google-closure-compiler-js').webpack;

let plugins = [];

const isProduction = process.env.NODE_ENV === 'prod';

const compilerPlugin = new ClosureCompiler({
	options: {
		languageIn: 'ECMASCRIPT6',
		languageOut: 'ECMASCRIPT6',
		compilationLevel: 'WHITESPACE_ONLY',
		warningLevel: 'QUIET',
		jsCode: ['./site/static/js/all.js']
	},
});

const cleanWebpackPlugin = new CleanWebpackPlugin(['site/static']);
const hotModulePlugin = new webpack.HotModuleReplacementPlugin();

if (isProduction) {
	plugins.push(compilerPlugin);
} else {
	plugins.push(hotModulePlugin);
}

module.exports = {
	entry: ['./src/index.js' /*, './src/scss/all.scss'*/],

	output: {
		path: path.resolve(__dirname, './site/'),
		filename: './static/js/all.js'
	},

	devtool: 'source-map',

	devServer: {
		contentBase: 'site',
		compress: true,
		port: 9001,
		hotOnly: true
	},

	module: {
		rules: [
			{
				test: /\.js$/,
				use: [
					{loader: 'babel-loader'},
				],
				exclude: /node_modules/
			}
		]
	},

	resolve: {
	 extensions: ['.tsx', '.ts', '.js']
	},

	plugins: plugins
};
