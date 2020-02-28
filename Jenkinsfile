DOCKER_IMAGE_NAME = 'gcr.io/organic-storm-201412/fetch-ledger-develop:v0.4.5'
STATIC_ANALYSIS_IMAGE = 'gcr.io/organic-storm-201412/ledger-ci-clang-tidy:v0.1.3'

HIGH_LOAD_NODE_LABEL = 'ledger'
MACOS_NODE_LABEL = 'osx'

enum Platform
{
  MACOS_CLANG('Clang',   'clang',     'clang++',     ''),
  CLANG6     ('Clang 6', 'clang-6.0', 'clang++-6.0', 'gcr.io/organic-storm-201412/ledger-ci-clang6:v0.1.3'),
  CLANG7     ('Clang 7', 'clang-7',   'clang++-7',   'gcr.io/organic-storm-201412/ledger-ci-clang7:v0.1.3'),
  GCC7       ('GCC 7',   'gcc-7',     'g++-7',       'gcr.io/organic-storm-201412/ledger-ci-gcc7:v0.1.3'),
  GCC8       ('GCC 8',   'gcc-8',     'g++-8',       'gcr.io/organic-storm-201412/ledger-ci-gcc8:v0.1.3')

  public Platform(label, cc, cxx, image)
  {
    this.label = label
    this.env_cc = cc
    this.env_cxx = cxx
    this.docker_image = image
  }

  public final String label
  public final String env_cc
  public final String env_cxx
  public final String docker_image
}

LINUX_PLATFORMS_CORE = [
  Platform.CLANG6,
  Platform.GCC7]

LINUX_PLATFORMS_AUX = [
  Platform.CLANG7,
  Platform.GCC8]

enum Configuration
{
  DEBUG  ('Debug'),
  RELEASE('Release')

  public Configuration(label)
  {
    this.label = label
  }

  public final String label
}

def run_full_build()
{
  return true
}

def set_up_pipenv()
{
  sh "pipenv install --dev --deploy"
}

def static_analysis(Configuration config)
{
  return {
    stage('Static Analysis') {
      node(HIGH_LOAD_NODE_LABEL) {
        stage('SCM Static Analysis') {
          checkout scm
        }

        docker.image(STATIC_ANALYSIS_IMAGE).inside {
          stage('Set Up Static Analysis') {
            set_up_pipenv()
          }

          stage('Run Static Analysis') {
            sh "pipenv run ./scripts/ci-tool.py --lint ${config.label}"
          }
        }
      }
    }
  }
}

def stage_name_suffix(Platform platform, Configuration config)
{
  return "${platform.label} ${config.label}"
}

def build_stage(Platform platform, Configuration config)
{
  return {
    stage("Build ${stage_name_suffix(platform, config)}") {
      def concurrency_opt = platform == Platform.MACOS_CLANG
        ? "-j4"
        : "";

      sh "pipenv run ./scripts/ci-tool.py -B ${concurrency_opt} ${config.label}"
    }
  }
}

def fast_tests_stage(Platform platform, Configuration config)
{
  return {
    stage("Unit Tests ${stage_name_suffix(platform, config)}") {
      sh "pipenv run ./scripts/ci-tool.py -T ${config.label}"
    }

    stage("Etch Lang Tests ${stage_name_suffix(platform, config)}") {
      sh "pipenv run ./scripts/ci-tool.py -L ${config.label}"
    }
  }
}

def slow_tests_stage(Platform platform, Configuration config)
{
  return {
    stage("Slow Tests ${stage_name_suffix(platform, config)}") {
      sh "pipenv run ./scripts/ci-tool.py -S ${config.label}"
    }

    stage("Integration Tests ${stage_name_suffix(platform, config)}") {
      sh "pipenv run ./scripts/ci-tool.py -I ${config.label}"
    }

    stage("End-to-End Tests ${stage_name_suffix(platform, config)}") {
      sh "pipenv run ./scripts/ci-tool.py -E ${config.label}"
    }
  }
}

def _create_build(
  Platform platform,
  Configuration config,
  node_label,
  stages_fn,
  run_build_fn)
{
  def environment = {
    return [
      "CC=${platform.env_cc}",
      "CXX=${platform.env_cxx}"
    ]
  }

  return {
    stage(stage_name_suffix(platform, config)) {
      node(node_label) {
        timeout(120) {
          stage("SCM ${stage_name_suffix(platform, config)}") {
            checkout scm
          }

          withEnv(environment()) {
            run_build_fn(stages_fn(platform, config))
          }
        }
      }
    }
  }
}

fast_run = { platform_, config_ ->
  return {
    build_stage(platform_, config_)()
    fast_tests_stage(platform_, config_)()
  }
}

full_run = { platform_, config_ ->
  return {
    build_stage(platform_, config_)()
    fast_tests_stage(platform_, config_)()
    slow_tests_stage(platform_, config_)()
  }
}


def create_docker_build(Platform platform, Configuration config, stages)
{
  def build = { build_stages ->
    docker.image(platform.docker_image).inside {
      stage("Set Up ${stage_name_suffix(platform, config)}") {
        set_up_pipenv()
      }

      build_stages()
    }
  }

  return _create_build(
    platform,
    config,
    HIGH_LOAD_NODE_LABEL,
    stages,
    build)
}

def create_macos_build(Platform platform, Configuration config)
{
  def build = { build_stages ->
    try {
      stage("Set Up ${stage_name_suffix(platform, config)}") {
        set_up_pipenv()
      }

      build_stages()
    } finally {
      stage("Tear Down ${stage_name_suffix(platform, config)}") {
        sh "pipenv --rm"
      }
    }
  }

  return _create_build(
    platform,
    config,
    MACOS_NODE_LABEL,
    fast_run,
    build)
}

def run_builds_in_parallel()
{
  def stages = [:]

  for (config in Configuration.values())
  {
    for (platform in LINUX_PLATFORMS_CORE)
    {
      stages["${platform.label} ${config.label}"] = create_docker_build(
        platform,
        config,
        full_run)
    }

    // Only run macOS builds on master and head branches
    if (run_full_build())
    {
      stages["macOS ${Platform.MACOS_CLANG.label} ${config.label}"] = create_macos_build(
        Platform.MACOS_CLANG,
        config)
    }
  }

//   for (config in (run_full_build() ? Configuration.values() : [Configuration.RELEASE]))
//   {
//     for (platform in LINUX_PLATFORMS_AUX)
//     {
//       stages["${platform.label} ${config.label}"] = create_docker_build(
//         platform,
//         config,
//         run_full_build() ? full_run : fast_run)
//     }
//   }

  if (run_full_build())
  {
    stages['Static Analysis'] = static_analysis(Configuration.DEBUG)
  }

  stage('Build and Test') {
    // Execute stages
    parallel stages
  }
}

def run_basic_checks()
{
  stage('Basic Checks') {
    node {
      stage('SCM Basic Checks') {
        checkout scm
      }

      docker.image(DOCKER_IMAGE_NAME).inside {
        stage('Set Up Basic Checks') {
          set_up_pipenv()
        }

        stage('Style Check') {
          sh 'pipenv run ./scripts/apply_style.py -d'
        }

        stage('Circular Dependencies') {
          sh 'mkdir -p build-deps && cd build-deps && cmake ../'
          sh 'pipenv run ./scripts/detect-circular-dependencies.py build-deps/'
        }
      }
    }
  }
}

def main()
{
  timeout(180) {
    run_basic_checks()
    run_builds_in_parallel()
  }
}

// Entry point
main()
