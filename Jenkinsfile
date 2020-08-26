#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos7',
        'centos8',
        'xenial',
        'bionic',
        'stretch',
        'buster'
    ],
    target_repos: [
        'wallarm-node': [
		release: '2.17',
		dev: true
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)
